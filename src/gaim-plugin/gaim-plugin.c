/*
 * Copyright 2004, 2005 Gregor Richards
 *
 * This file is part of DirectNet.
 *
 *    DirectNet is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DirectNet is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DirectNet; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef WIN32

#include <glib-object.h>
#include <glib/gtypes.h>

// Badly get rid of a mystery error...
#define GTranslateFunc int
#include <glib/goption.h>
#undef GTranslateFunc

#endif

#include <glib/gmain.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
extern char **environ;

#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "gaim-plugin.h"
#include "enc.h"
#include "hash.h"
// Make sure we get DN server.h, not Gaim server.h
#include "../server.h"
#include "ui.h"

static GaimPlugin *my_protocol = NULL;
static GaimAccount *my_account = NULL; /* only allow one login - icky :-P */
static GaimConnection *my_gc = NULL;

struct BuddyList {
    GaimBuddy *buddy;
    struct BuddyList *next;
};

struct BuddyList *bltop = NULL;

static GaimPluginProtocolInfo prpl_info = {
    0, /* OPT_PROTO_NO_PASSWORD */
    NULL,
    NULL,
    NO_BUDDY_ICONS,
    gp_listicon,
    NULL,
    NULL,
    NULL,
    gp_awaystates, /* away states */
    NULL,
    gp_chatinfo,
    gp_chatinfo_defaults,
    gp_login, /* login */
    gp_close, /* close */
    gp_sendim, /* send IM */
    NULL, /* set info */
    NULL, /* send "typing" */
    NULL, /* get info */
    gp_setaway, /* set away */
    NULL,
    NULL,
    gp_addbuddy, /* add buddy */
    NULL,
    gp_removebuddy, /* remove buddy */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gp_joinchat, /* join a chat */
    NULL,
    NULL,
    NULL, /*gp_simplechatinvite,  chat invite by sending a message */
    gp_leavechat, /* leave a chat */
    NULL,
    gp_saychat, /* chat send */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static GaimPluginInfo info = {
    GAIM_PLUGIN_MAGIC,
    GAIM_MAJOR_VERSION,
    GAIM_MINOR_VERSION,
    GAIM_PLUGIN_PROTOCOL,
    NULL, /* ui requirement */
    0, /* flags */
    NULL, /* deps */
    GAIM_PRIORITY_DEFAULT,
    
    "prpl-directnet", /* id */
    "DirectNet", /* name */
    "b0.6", /* version */
    
    N_("DirectNet Protocol Plugin"), /* summary */
    N_("DirectNet Protocol Plugin"), /* full ;) description */
    
    "Gregor Richards", /* author */
    "http://directnet.sourceforge.net", /* homepage */
    
    NULL, /* load */
    NULL, /* unload */
    NULL, /* destroy */
    
    NULL, /* ui info */
    &prpl_info, /* extra info */
    NULL,
    NULL
};

char *chat_ids[256];
pthread_mutex_t chat_lock;

int regChat(char *name)
{
    int i;
    
    pthread_mutex_lock(&chat_lock);
    for (i = 0; i < 255 && chat_ids[i]; i++);
    chat_ids[i] = strdup(name);
    pthread_mutex_unlock(&chat_lock);
    
    return i;
}

int chatByName(char *name)
{
    int i;
    
    pthread_mutex_lock(&chat_lock);
    for (i = 0; i < 256; i++) {
        if (chat_ids[i] && !strcmp(name, chat_ids[i])) {
            pthread_mutex_unlock(&chat_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&chat_lock);
    return -1;
}

pthread_mutex_t ipclock;
GMainContext *ctx;
#define IPC_MSG 1
struct ipc_msg {
    char *from;
    char *msg;
    char *authmsg;
};
#define IPC_STATE 2
struct ipc_state {
    char *who;
    int state;
};
#define IPC_CHAT_MSG 3
struct ipc_chat_msg {
    int id;
    char *who;
    char *chan;
    char *msg;
};
#define IPC_CHAT_JOIN 4
struct ipc_chat_join {
    int id;
    char *chan;
};
#define IPC_AUTH_ASK 5
struct ipc_auth_ask {
    char *from;
    char *msg;
    char *q;
};


gboolean idle_got_im(struct ipc_msg *ma) 
{
    char *dispnm = (char *) malloc((strlen(ma->from) + strlen(ma->authmsg) + 4) * sizeof(char));
    sprintf(dispnm, "%s [%s]", ma->from, ma->authmsg);
    
    serv_got_alias(my_gc, ma->from, dispnm);
    serv_got_im(my_gc, ma->from, ma->msg, 0, time(NULL));
  
    free(ma->from);
    free(ma->msg);
    free(ma->authmsg);
    free(ma);
    free(dispnm);

    pthread_mutex_unlock(&ipclock);

    return FALSE;
}

gboolean idle_set_state(struct ipc_state *sa)
{
    serv_got_update(my_gc, sa->who, sa->state, 0, 0, 0, 0);
    
    free(sa->who);
    free(sa);
    
    pthread_mutex_unlock(&ipclock);
    
    return FALSE;
}

gboolean idle_chat_msg(struct ipc_chat_msg *cma)
{
    serv_got_chat_in(my_gc, cma->id, cma->who, 0, cma->msg, time(NULL));

    free(cma->who);
    free(cma->msg);
    free(cma);
    
    pthread_mutex_unlock(&ipclock);
    
    return FALSE;
}           

gboolean idle_chat_join(struct ipc_chat_join *cja)
{
    serv_got_joined_chat(my_gc, cja->id, cja->chan);

    free(cja->chan);
    free(cja);
    
    pthread_mutex_unlock(&ipclock);

    return FALSE;
}

gboolean idle_auth_ask_y_cb(struct ipc_auth_ask *aaa)
{
    authImport(aaa->msg);
    free(aaa->from);
    free(aaa->msg);
    free(aaa->q);
    free(aaa);
    return FALSE;
}

gboolean idle_auth_ask_n_cb(struct ipc_auth_ask *aaa)
{
    free(aaa->from);
    free(aaa->msg);
    free(aaa->q);
    free(aaa);
    return FALSE;
}

gboolean idle_auth_ask(struct ipc_auth_ask *aaa)
{
    gaim_request_accept_cancel(my_gc, "Authentication", aaa->q,
                               NULL, 1, aaa,
                               idle_auth_ask_y_cb, idle_auth_ask_n_cb);
    
    pthread_mutex_unlock(&ipclock);
    
    return FALSE;
}

void ipc_got_im(char *who, char *msg, char *authmsg)
{
    struct ipc_msg *a = (struct ipc_msg *) malloc(sizeof(struct ipc_msg));

    a->from = strdup(who);
    a->msg = strdup(msg);
    a->authmsg = strdup(authmsg);
    
    pthread_mutex_lock(&ipclock);
    
    g_idle_add((GSourceFunc) idle_got_im, a);
}

void ipc_set_state(char *who, int state)
{
    struct ipc_state *a = (struct ipc_state *) malloc(sizeof(struct ipc_state));

    a->who = strdup(who);
    a->state = state;

    pthread_mutex_lock(&ipclock);
    
    g_idle_add((GSourceFunc)idle_set_state, a);
}

void ipc_chat_msg(int id, char *who, char *chan, char *msg)
{
    struct ipc_chat_msg *a = (struct ipc_chat_msg *) malloc(sizeof(struct ipc_chat_msg));

    a->id = id;
    a->who = strdup(who);
    a->chan = strdup(chan);
    a->msg = strdup(msg);
    
    pthread_mutex_lock(&ipclock);
    
    g_idle_add((GSourceFunc)idle_chat_msg, a);
}

void ipc_chat_join(int id, char *chan)
{
    struct ipc_chat_join *a = (struct ipc_chat_join *) malloc(sizeof(struct ipc_chat_join));

    a->id = id;
    a->chan = strdup(chan);
    
    pthread_mutex_lock(&ipclock);
    
    g_idle_add((GSourceFunc)idle_chat_join, a);
}

void ipc_auth_ask(char *from, char *msg, char *q)
{
    struct ipc_auth_ask *a = (struct ipc_auth_ask *) malloc(sizeof(struct ipc_auth_ask));
    
    a->from = strdup(from);
    a->msg = strdup(msg);
    
    a->q = (char *) malloc((strlen(from) + strlen(q) + 51) * sizeof(char));
    sprintf(a->q, "%s has asked you to import the key %s.  Do you accept?", from, q);
    
    pthread_mutex_lock(&ipclock);
    
    g_idle_add((GSourceFunc)idle_auth_ask, a);
}

static void init_plugin(GaimPlugin *plugin)
{
    GaimAccountOption *option;
    
    option = gaim_account_option_string_new("Auth User", "authuser", "");
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
    
    my_protocol = plugin;
}

GAIM_INIT_PLUGIN(DirectNet, init_plugin, info);

int uiInit(int argc, char ** argv, char **envp)
{
    // This is the most basic UI
    int charin, ostrlen;
    char cmdbuf[32256];
    char *homedir, *dnhomefile;
    
    // Always start by finding encryption
    if (findEnc(envp) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return -1;
    }
    
    /* Then authentication */
    if (!authInit()) {
        printf("Authentication failed to initialize!\n");
        return -1;
    }
    
    // Lock chat IDs
    pthread_mutex_init(&chat_lock, NULL);
    memset(chat_ids, 0, sizeof(int) * 256);
    
    // And create the key
    encCreateKey();
    
    return 0;
}

void uiDispMsg(char *from, char *msg, char *authmsg)
{
    while (!uiLoaded) sleep(0);
    
    ipc_got_im(from, msg, authmsg);
}

void uiAskAuthImport(char *from, char *msg, char *sig)
{
    while (!uiLoaded) sleep(0);
    
    ipc_auth_ask(from, msg, sig);
}

void uiDispChatMsg(char *chat, char *from, char *msg)
{
    int id;
    char *fullname = (char *) malloc(strlen(chat) + 2);
    
    sprintf(fullname, "#%s", chat);
    
    id = chatByName(fullname);
    
    if (id != -1) {
        ipc_chat_msg(chatByName(fullname), from, fullname, msg);
    }
    
    free(fullname);
}

void uiEstConn(char *from)
{
}

void uiEstRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    
    GaimBuddy *buddy;

    /* if they're on the buddy list, update them */
    buddy = gaim_find_buddy(my_account, from);
    if (buddy && buddy->present == 0) {
        ipc_set_state(from, 1);
    } else if (!buddy) {
        ipc_got_im(from, "[DirectNet]: Route established.", "sys");
    }
}

void uiLoseConn(char *from)
{
    uiLoseRoute(from);
}

void uiLoseRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    
    GaimBuddy *buddy;

    /* if they're on the buddy list, update them */
    buddy = gaim_find_buddy(my_account, from);
    if (buddy && buddy->present != 0) {
        ipc_set_state(from, 0);
    }
}

void uiNoRoute(char *to)
{
}

static const char *gp_listicon(GaimAccount *a, GaimBuddy *b)
{
    return "DirectNet";
}

static GList *gp_chatinfo(GaimConnection *gc)
{
    GList *m = NULL;
    struct proto_chat_entry *pce;

    pce = g_new0(struct proto_chat_entry, 1);
    pce->label = "_Room:";
    pce->identifier = "room";
    m = g_list_append(m, pce);

    return m;
}

GHashTable *gp_chatinfo_defaults(GaimConnection *gc, const char *chat_name)
{
    GHashTable *defaults;

    defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    if (chat_name != NULL)
        g_hash_table_insert(defaults, "room", g_strdup(chat_name));


    return defaults;
}

static GList *gp_awaystates(GaimConnection *gc)
{
    GList *m = NULL;

    m = g_list_append(m, GAIM_AWAY_CUSTOM);
    m = g_list_append(m, _("Back"));

    return m;
}

static void gp_login(GaimAccount *account)
{
    char *argv[] = {"plugin", NULL};
    GaimConnection *gc;
    
    // The nick should be defined
    strcpy(dn_name, account->username);
    dn_name_set = 1; 
    
    my_account = account;
    
    /* set auth password */
    if (authNeedPW()) {
        char *nm, *pswd;
        int osl;
        
        /* name */
        nm = strdup(gaim_account_get_string(account, "authname", ""));
        if (nm[0] == '\0') {
            free(nm);
            nm = strdup(dn_name);
        }
        
        /* password */
        pswd = (char *) gaim_account_get_password(my_account);
        if (pswd) {
            pswd = strdup(pswd);
            authSetPW(nm, pswd);
            free(pswd);
        }
        
        free(nm);
    }
    
    // Prepare the IPC pipe
    pthread_mutex_init(&ipclock, NULL);
    
    /* call "main" */
    pluginMain(1, argv, environ);

    // pthreads are magical
    sleep(1);
    
    gc = gaim_account_get_connection(account);
    gaim_connection_set_state(gc, GAIM_CONNECTED);
    gaim_connection_update_progress(gc, _("Connected"), 0, 1);
    
    my_gc = gc;

    serv_finish_login(gc);
    
    // OK, the UI is ready
    uiLoaded = 1;
}

static void gp_close(GaimConnection *gc)
{
    struct BuddyList *cur, *next;
    int i, curfd;

    if (gc == my_gc) {
        /* OK, try to free and delete everything we can */
        cur = bltop;
        bltop = NULL;
        while (cur) {
            next = cur->next;
            free(cur);
            cur = next;
        }
        
        my_protocol = NULL;
        my_account = NULL;
        my_gc = NULL;
        
        serverPthread ? pthread_kill(*serverPthread, SIGTERM) : 0;
        
        for (i = 0; i < onpthread; i++) {
            pthreads[i] ? pthread_kill(*(pthreads[i]), SIGTERM) : 0;
        }
        
        if (close(server_sock) < 0) {
            perror("close");
        }
        
        for (curfd = 0; curfd < DN_MAX_CONNS && curfd < onfd; curfd++)
        {
            if (fds[curfd]) {
                close(fds[curfd]);
            }
        }
    
        free(fds);
        free(pipe_fds);
        free(pipe_locks);
        free(pthreads);
    }
}

static int gp_sendim(GaimConnection *gc, const char *who, const char *message, GaimConvImFlags flags)
{
    while (!uiLoaded) sleep(0);
    
    if (!strncmp(who, "con:", 4)) goto exit;
    
    // don't let people talk to themself
    if (!strcmp(who, dn_name)) goto exit;
    
    char *nwho = strdup(who);
    char *nmsg = strdup(message);
    sendMsg(nwho, nmsg);
    free(nwho);
    free(nmsg);
 exit:
    return 1;
}

static void gp_setaway(GaimConnection *gc, const char *state, const char *text)
{
    char *ntext = text ? strdup(text) : strdup("I'm away right now.");

	setAway(ntext);
    free(ntext);

}

void *waitConn(void *to_vp)
{
    struct BuddyList *cur;
    GaimBuddy *to = (GaimBuddy *) to_vp;
    char *route;

    while (!uiLoaded) sleep(0);

    // don't let people talk to themself
    if (!strcmp(to->name, dn_name)) return NULL;
    
    if (!strncmp(to->name, "con:", 4)) {
        if (establishConnection(to->name + 4)) {
            if (to->present == 0) {
                ipc_set_state(to->name, 1);
            }
        }
    } else {
        /* if we have a route, mark them as online, otherwise, send a find */
        route = hashSGet(dn_routes, to->name);
        if (route) {
            uiEstRoute(to->name);
        } else {
            sendFnd(to->name);
        }
        return NULL;
    }
    
    /* since this is a connection, try now to find offline buddies */
    cur = bltop;
    while (cur) {
        if (cur->buddy->present == 0 && strncmp(cur->buddy->name, "con:", 4)) {
            sendFnd(cur->buddy->name); /* FIXME: this should not go on all connections */
        }
        cur = cur->next;
    }
    return NULL;
}

static void gp_addbuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group)
{
    pthread_t newthread;
    char *route;

    /* there are special "buddies" that are connections: con:host */
    if (!strncmp(buddy->name, "con:", 4)) {
        pthread_create(&newthread, NULL, waitConn, buddy);
    } else {
        struct BuddyList *cur;

        /* if it's in the list, we're done */
        cur = bltop;
        while (cur) {
            if (!strcmp(cur->buddy->name, buddy->name)) return;
            cur = cur->next;
        }
        
        /* otherwise, add to the list */
        if (bltop) {
            cur = bltop;
            while (cur->next) cur = cur->next;
            cur->next = (struct BuddyList *) malloc(sizeof(struct BuddyList));
            cur = cur->next;
        } else {
            bltop = (struct BuddyList *) malloc(sizeof(struct BuddyList));
            cur = bltop;
        }

        cur->next = NULL;
        cur->buddy = buddy;
        
        /* and try to find it */
        pthread_create(&newthread, NULL, waitConn, buddy);
    }
}

static void gp_removebuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group)
{
    /* put code here 8-D */
}

static void gp_joinchat(GaimConnection *gc, GHashTable *data)
{

  char *name = g_hash_table_lookup(data, "room");
    if (name[0] == '#') {
        joinChat(name + 1);
        ipc_chat_join(regChat(name), name);
    } else {
        char *fullname = (char *) malloc(strlen(name) + 2);
        joinChat(name);
        sprintf(fullname, "#%s", name);
        ipc_chat_join(regChat(fullname), fullname);
        free(fullname);
    }
}

static void gp_simplechatinvite() {}

static void gp_leavechat(GaimConnection *gc, int id)
{
    GaimConversation *gconv = gaim_find_chat(gc, id);
    leaveChat(gconv->name + 1);
    id = chatByName(gconv->name);

    pthread_mutex_lock(&chat_lock);
    free(chat_ids[id]);
    chat_ids[id] = NULL;
    pthread_mutex_unlock(&chat_lock);
}

static int gp_saychat(GaimConnection *gc, int id, const char *msg)
{
    GaimConversation *gconv = gaim_find_chat(gc, id);
    char *omsg = strdup(msg);

    sendChat(gconv->name + 1, omsg);
    ipc_chat_msg(id, dn_name, gconv->name, omsg);
    
    free(omsg);

    return 1;
}
