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

#include <stdio.h>

#ifdef WIN32
#define pipe(a) _pipe((a), 512, _O_BINARY | _O_NOINHERIT)

#include <glib-object.h>
#include <glib/gtypes.h>

// Badly get rid of a mystery error...
#define GTranslateFunc int
#include <glib/goption.h>
#undef GTranslateFunc

// fgets on Windows won't delimit on \n
char *fgets_win32(char *s, int size, FILE *stream)
{
    int i, c;
    for (i = 0; i < size-1; i++) {
        c = getc(stream);
	s[i] = c;
	if (c == '\n') {
	    s[i+1] = '\0';
	    return s;
	}
    }
    return s;
}
#define fgets fgets_win32

#endif

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
extern char **environ;

#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "gaim-plugin.h"
#include "gpg.h"
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
    OPT_PROTO_NO_PASSWORD,
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
    NULL, /* close */
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
    NULL, /*gp_simplechatinvite, /* chat invite by sending a message */
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
    "a0.4", /* version */
    
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

int ipcpipe[2];
FILE *ipcin, *ipcout;
pthread_mutex_t ipclock;
#define IPC_MSG 1
struct ipc_msg {
    char *from;
    char *msg;
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

void ipc_got_im(char *who, char *msg)
{
    struct ipc_msg *a = (struct ipc_msg *) malloc(sizeof(struct ipc_msg));
    a->from = strdup(who);
    a->msg = strdup(msg);
    pthread_mutex_lock(&ipclock);
    fprintf(ipcout, "%c%p\n", IPC_MSG, a);
    fflush(ipcout);
}

void ipc_set_state(char *who, int state)
{
    struct ipc_state *a = (struct ipc_state *) malloc(sizeof(struct ipc_state));
    a->who = strdup(who);
    a->state = state;
    pthread_mutex_lock(&ipclock);
    fprintf(ipcout, "%c%p\n", IPC_STATE, a);
    fflush(ipcout);
}

void ipc_chat_msg(int id, char *who, char *chan, char *msg)
{
    struct ipc_chat_msg *a = (struct ipc_chat_msg *) malloc(sizeof(struct ipc_chat_msg));
    a->id = id;
    a->who = strdup(who);
    a->chan = strdup(chan);
    a->msg = strdup(msg);
    pthread_mutex_lock(&ipclock);
    fprintf(ipcout, "%c%p\n", IPC_CHAT_MSG, a);
    fflush(ipcout);
}

void ipc_chat_join(int id, char *chan)
{
    struct ipc_chat_join *a = (struct ipc_chat_join *) malloc(sizeof(struct ipc_chat_join));
    a->id = id;
    a->chan = strdup(chan);
    pthread_mutex_lock(&ipclock);
    fprintf(ipcout, "%c%p\n", IPC_CHAT_JOIN, a);
    fflush(ipcout);
}

static void init_plugin(GaimPlugin *plugin)
{
    my_protocol = plugin;
}

GAIM_INIT_PLUGIN(DirectNet, init_plugin, info);

int uiInit(int argc, char ** argv, char **envp)
{
    // This is the most basic UI
    int charin, ostrlen;
    char cmdbuf[32256];
    char *homedir, *dnhomefile;
    
    // Always start by finding GPG
    if (findGPG(envp) == -1) {
        printf("GPG was not found on your PATH!\n");
        return -1;
    }
    
    // Lock chat IDs
    pthread_mutex_init(&chat_lock, NULL);
    memset(chat_ids, 0, sizeof(int) * 256);
    
    // And create the key
    gpgCreateKey();
}

void uiDispMsg(char *from, char *msg)
{
    while (!uiLoaded) sleep(0);

    ipc_got_im(from, msg);
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
        ipc_got_im(from, "[DirectNet]: Route established.");
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

static void gp_callback(gpointer data, gint source, GaimInputCondition condition)
{
    /* grab our input line */
    char inpline[1024];
    struct ipc_msg *ma;
    struct ipc_state *sa;
    struct ipc_chat_msg *cma;
    struct ipc_chat_join *cja;
    
    while (!uiLoaded) sleep(0);
    
    inpline[1023] = '\0';
    
    fgets(inpline, 1023, ipcin);
    
    switch (inpline[0]) {
        case IPC_MSG:
            if (sscanf(inpline + 1, "%p", &ma) < 1) return;
            serv_got_im(my_gc, ma->from, ma->msg, 0, time(NULL));
            free(ma->from);
            free(ma->msg);
            free(ma);
            break;
            
        case IPC_STATE:
            if (sscanf(inpline + 1, "%p", &sa) < 1) return;
            serv_got_update(my_gc, sa->who, sa->state, 0, 0, 0, 0);
            free(sa->who);
            free(sa);
            break;
            
        case IPC_CHAT_MSG:
            if (sscanf(inpline + 1, "%p", &cma) < 1) return;
            serv_got_chat_in(my_gc, cma->id, cma->who, 0, cma->msg, time(NULL));
            free(cma->who);
            free(cma->msg);
            free(cma);
            break;
            
        case IPC_CHAT_JOIN:
            if (sscanf(inpline + 1, "%p", &cja) < 1) return;
            serv_got_joined_chat(my_gc, cja->id, cja->chan);
            free(cja->chan);
            free(cja);
            break;
    }
    
    pthread_mutex_unlock(&ipclock);
}
    
static void gp_login(GaimAccount *account)
{
    char *argv[] = {"plugin", NULL};
    GaimConnection *gc;
    
    // The nick should be defined
    strcpy(dn_name, account->username);
    dn_name_set = 1; 
    
    my_account = account;

    // Prepare the IPC pipe
    pipe(ipcpipe);
    ipcin = fdopen(ipcpipe[0], "r");
    ipcout = fdopen(ipcpipe[1], "w");
    pthread_mutex_init(&ipclock, NULL);
    gaim_input_add(ipcpipe[0], GAIM_INPUT_READ, gp_callback, gc);
    
    /* call "main" */
    pluginMain(1, argv, environ);
    
    gc = gaim_account_get_connection(account);
    gaim_connection_set_state(gc, GAIM_CONNECTED);
    gaim_connection_update_progress(gc, _("Connected"), 0, 1);
    
    my_gc = gc;

    serv_finish_login(gc);
    
    // OK, the UI is ready
    uiLoaded = 1;
}

static int gp_sendim(GaimConnection *gc, const char *who, const char *message, GaimConvImFlags flags)
{
    while (!uiLoaded) sleep(0);
    
    if (!strncmp(who, "con:", 4)) return 1;
    
    // don't let people talk to themself
    if (!strcmp(who, dn_name)) return;
    
    char *nwho = strdup(who);
    char *nmsg = strdup(message);
    sendMsg(nwho, nmsg);
    free(nwho);
    free(nmsg);
    return 1;
}

static void gp_setaway(GaimConnection *gc, const char *state, const char *text)
{
    char *ntext = strdup(text);
    setAway(ntext);
    free(ntext);
}

void *waitConn(void *to_vp)
{
    struct BuddyList *cur;
    GaimBuddy *to = (GaimBuddy *) to_vp;

    while (!uiLoaded) sleep(0);

    // don't let people talk to themself
    if (!strcmp(to->name, dn_name)) return;
    
    if (!strncmp(to->name, "con:", 4)) {
        if (establishConnection(to->name + 4)) {
            if (to->present == 0) {
                ipc_set_state(to->name, 1);
            }
        }
    } else {
        sendFnd(to->name);
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
    ipc_chat_msg(id, dn_name, gconv->name, msg);
    
    free(omsg);
    
    return 1;
}
