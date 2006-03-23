/*
 * Copyright 2004, 2005, 2006  Gregor Richards
 * Copyright 2006  "Solarius"
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

#include <set>
#include <string>
using namespace std;

#ifdef WIN32

extern "C" {
#include <glib-object.h>
#include <glib/gtypes.h>

// Badly get rid of a mystery error...
#define GTranslateFunc int
#include <glib/goption.h>
#undef GTranslateFunc
}

#endif

#include <glib/gmain.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
extern char **environ;

#include "auth.h"
#include "client.h"
// Make sure we get DN connection.h, not Gaim connection.h
#include "../connection.h"
#include "directnet.h"
#include "dnconfig.h"
#include "gaim-plugin.h"
#include "enc.h"
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
#ifndef __WIN32
    OPT_PROTO_PASSWORD_OPTIONAL,
#else
    OPT_PROTO_NO_PASSWORD, /* authentication unsupported on Windows */
#endif
    NULL,
    NULL,
    NO_BUDDY_ICONS,
    gp_listicon,
    NULL,
    NULL,
    NULL,
    gp_statustypes, /* away states */
    NULL,
    gp_chatinfo,
    gp_chatinfo_defaults,
    gp_login, /* login */
    gp_close, /* close */
    gp_sendim, /* send IM */
    NULL, /* set info */
    NULL, /* send "typing" */
    NULL, /* get info */
    gp_setstatus, /* set status */
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
    "1.0.1", /* version */
    
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

int regChat(const char *name)
{
    int i;
    
    for (i = 0; i < 255 && chat_ids[i]; i++);
    chat_ids[i] = strdup(name);
    
    return i;
}

int chatByName(const char *name)
{
    int i;
    
    for (i = 0; i < 256; i++) {
        if (chat_ids[i] && !strcmp(name, chat_ids[i])) {
            return i;
        }
    }
    return -1;
}

void got_im(const char *who, const char *msg, const char *authmsg, int away)
{
    char *dispnm = (char *) malloc((strlen(who) + strlen(authmsg) + 11) * sizeof(char));
    sprintf(dispnm, "%s [%s]%s", who, authmsg, away ? " [away]" : "");
    
    serv_got_alias(my_gc, who, dispnm);
    serv_got_im(my_gc, who, msg, 0, time(NULL));
  
    free(dispnm);
}

void set_state(const char *who, int state)
{
    gaim_prpl_got_user_status(my_account, who,
                              (state == 1) ? "available" : "offline", NULL);
}

void chat_msg(int id, const char *who, const char *chan, const char *msg)
{
    serv_got_chat_in(my_gc, id, who, 0, msg, time(NULL));
}           

void chat_join(int id, const char *chan)
{
    serv_got_joined_chat(my_gc, id, chan);
}

gboolean auth_ask_y_cb(char *msg)
{
    authImport(msg);
    free(msg);
    return FALSE;
}

gboolean auth_ask_n_cb(char *msg)
{
    free(msg);
    return FALSE;
}

void auth_ask(const char *from, const char *msg, const char *q)
{
    char *qa, *qb;
    int i, j, l;
    
    qa = (char *) malloc((strlen(from) + strlen(q) + 53) * sizeof(char));
    sprintf(qa, "%s has asked you to import the key '%s'.  Do you accept?", from, q);
    
    /* No <s or >s allowed! */
    l = strlen(qa) + 1;
    qb = (char *) malloc(l * sizeof(char));
    
    j = 0;
    for (i = 0; qa[i]; i++) {
        switch (qa[i]) {
            case '<':
                qb = (char *) realloc(qb, l += 3);
                strncpy(qb + j, "&lt;", 4);
                j += 4;
                break;
            case '>':
                qb = (char *) realloc(qb, l += 3);
                strncpy(qb + j, "&gt;", 4);
                j += 4;
                break;
            default:
                qb[j] = qa[i];
                j++;
                break;
        }
    }
    qb[j] = '\0';
    free(qa);
    
    gaim_request_yes_no(my_gc, "Authentication", qb,
                        NULL, 1, strdup(msg),
                        auth_ask_y_cb, auth_ask_n_cb);
    
    free(qb);
}

gboolean dnsndkey(GaimConversation *gc, gchar *cmd, gchar **args, gchar **error, void *data)
{
    sendAuthKey(gc->name);
    return FALSE;
}

void dnsndkey_vptr(GtkButton *butotn, void *name)
{
    sendAuthKey((char *) name);
}

/*void dn_crcon_button(GaimConversation *gc, gpointer data)
{
    GaimGtkConversation *gtkconv;
    gulong handle;
    GtkWidget *button = NULL;
    
    if((gtkconv = GAIM_GTK_CONVERSATION(gc)) == NULL)
        return;
    
    button = gaim_gtkconv_button_new("DNKey", "DN Key", "Send DirectNet Authentication Key",
                                     NULL, dnsndkey_vptr, gc->name);
    g_object_set_data(G_OBJECT(button), "conv", gc);
    gtk_box_pack_end(GTK_BOX(gtkconv->bbox), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_size_group_add_widget(gtkconv->sg, button);
}*/

static void init_plugin(GaimPlugin *plugin)
{
    GaimAccountOption *option;
    void *conv_handle = gaim_conversations_get_handle();
    
    option = gaim_account_option_string_new("Auth User", "authuser", "");
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
    
    /* register the key command * /
    gaim_cmd_register("k", "", 0, GAIM_CMD_FLAG_IM, "DirectNet", (GaimCmdFunc) dnsndkey,
                      "k:  Send your DirectNet authentication key.", NULL);*/
    
    /* register the function to add buttons to conversation windows */
    /*gaim_signal_connect(conv_handle, "conversation-created", plugin,
                        GAIM_CALLBACK(dn_crcon_button), NULL);*/
    
    my_protocol = plugin;
}

extern "C" {
    GAIM_INIT_PLUGIN(DirectNet, init_plugin, info);
}

void uiDispMsg(const string &from, const string &msg, const string &authmsg, int away)
{
    while (!uiLoaded) sleep(0);
    
    got_im(from.c_str(), msg.c_str(), authmsg.c_str(), away);
}

void uiAskAuthImport(const string &from, const string &msg, const string &sig)
{
    while (!uiLoaded) sleep(0);
    
    auth_ask(from.c_str(), msg.c_str(), sig.c_str());
}

void uiDispChatMsg(const string &chat, const string &from, const string &msg)
{
    int id;
    string fullname = "#" + chat;
    
    id = chatByName(fullname.c_str());
    
    if (id != -1) {
        chat_msg(id, from.c_str(), fullname.c_str(), msg.c_str());
    }
    
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Chat message received.\n");
}

void uiEstConn(const string &from)
{
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Connection established.\n");
}

void uiEstRoute(const string &from)
{
    //while (!uiLoaded) sleep(0);
    
    GaimBuddy *buddy;
    
    /* if they're on the buddy list, update them */
    buddy = gaim_find_buddy(my_account, from.c_str());
    if (buddy && !gaim_presence_is_online(buddy->presence)) {
        set_state(from.c_str(), 1);
    } else if (!buddy) {
        got_im(from.c_str(), "[DirectNet]: Route established.", "sys", 0);
    }

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Route established.\n");
}

void uiLoseConn(const string &from)
{
    uiLoseRoute(from);
}

void uiLoseRoute(const string &from)
{
    while (!uiLoaded) sleep(0);
    
    GaimBuddy *buddy;

    /* if they're on the buddy list, update them */
    buddy = gaim_find_buddy(my_account, from.c_str());
    if (buddy && gaim_presence_is_online(buddy->presence)) {
        set_state(from.c_str(), 0);
    }
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Lost route.\n");
}

void uiNoRoute(const string &to)
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

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Function gp_chatinfo.\n");
    return m;
}

GHashTable *gp_chatinfo_defaults(GaimConnection *gc, const char *chat_name)
{
    GHashTable *defaults;

    defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    if (chat_name != NULL)
        g_hash_table_insert(defaults, "room", g_strdup(chat_name));

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Function gp_chatinfo_defaults.\n");
    return defaults;
}

static GList *gp_statustypes(GaimAccount *ga)
{
    GList *m = NULL;

    m = g_list_append(m,
                      gaim_status_type_new_full(GAIM_STATUS_OFFLINE,
                                                "offline",
                                                _("Offline"), TRUE, TRUE, FALSE)
                      );
    m = g_list_append(m,
                      gaim_status_type_new_full(GAIM_STATUS_AVAILABLE,
                                                "available",
                                                _("Available"), TRUE, TRUE, FALSE)
                      );
    m = g_list_append(m,
                      gaim_status_type_new_with_attrs(GAIM_STATUS_AWAY,
                                                      "away",
                                                      _("Away"), TRUE, TRUE, FALSE,
                                                      "message", _("Message"),
                                                      gaim_value_new(GAIM_TYPE_STRING), NULL)
                      );

    return m;
}

static void gp_login(GaimAccount *account)
{
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Starting login....\n");

    int charin, ostrlen, i;
    char cmdbuf[32256];
    char *homedir, *dnhomefile;
    char *argv[] = {"plugin", NULL};
    GaimConnection *gc;
    GaimBuddy *buddy;
    
    // The nick should be defined
    strcpy(dn_name, account->username);
    
    my_account = account;
    
    // find our location - if prefix.h worked, it's LIBDIR
#ifndef LIBDIR
    // sad...
#define LIBDIR "/usr/lib/gaim"
#endif
    bindir = strdup(LIBDIR);
    
    dn_init(1, argv);

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Plugin initializing.\n");
    
    // Always start by finding encryption
    if (findEnc(environ) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return;
    }
    
    /* Then authentication */
    if (!authInit()) {
        printf("Authentication failed to initialize!\n");
        return;
    }
    
    // Lock chat IDs
    memset(chat_ids, 0, sizeof(int) * 256);
    
    // And create the key
    encCreateKey();
    
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
    
    // convert auto*'s into buddies
    set<string>::iterator aci, afi;
    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
        string sn = "con:" + *aci;
        
        if (!gaim_find_buddy(account, sn.c_str())) {
            buddy = gaim_buddy_new(account, sn.c_str(), NULL);
            gaim_blist_add_buddy(buddy, NULL, NULL, NULL);
        }
    }
    for (afi = dn_af_list->begin(); afi != dn_af_list->end(); afi++) {
        if (!gaim_find_buddy(account, afi->c_str())) {
            buddy = gaim_buddy_new(account, afi->c_str(), NULL);
            gaim_blist_add_buddy(buddy, NULL, NULL, NULL);
        }
    }
    
    dn_goOnline();
    
    gc = gaim_account_get_connection(account);
    gaim_connection_update_progress(gc, _("Connected"), 1, 2);
    gaim_connection_set_state(gc, GAIM_CONNECTED);
    
    my_gc = gc;
    
    // OK, the UI is ready
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "...Login OK!\n");
    uiLoaded = 1;
}

static void gp_close(GaimConnection *gc)
{
    // FIXME!!!
}

static int gp_sendim(GaimConnection *gc, const char *who, const char *message, GaimMessageFlags flags)
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
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Sent instant message.\n");
    exit:
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Leaving gp_sendim function.\n");
    return 1;
}

static void gp_setstatus(GaimAccount *account, GaimStatus *status)
{
    GaimStatusPrimitive primitive;
    
    if (!gaim_status_is_active(status))
        return;

    if (!gaim_account_is_connected(account))
        return;
    
    primitive = gaim_status_type_get_primitive(
        gaim_status_get_type(status));
    
    if (primitive == GAIM_STATUS_AVAILABLE) {
        setAway(NULL);
    } else if (primitive == GAIM_STATUS_AWAY) {
        const char *txt = gaim_status_get_attr_string(status, "message");
        string amsg;
        if (txt == NULL || txt[0] == '\0') {
            /* well, they're still away! */
            amsg = "Away";
        } else {
            amsg = txt;
        }
        
        setAway(&amsg);
    }
    
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Setting status.\n");
}

//Notice! This function do not (yet) have debug
void *waitConn(void *to_vp)
{
    struct BuddyList *cur;
    GaimBuddy *to = (GaimBuddy *) to_vp;
    char *route;

    while (!uiLoaded) sleep(0);

    // don't let people talk to themself
    if (!strcmp(to->name, dn_name)) return NULL;
    
    if (!strncmp(to->name, "con:", 4)) {
        establishConnection(to->name + 4);
    } else {
        /* if we have a route, mark them as online, otherwise, send a find */
        if (dn_routes->find(to->name) != dn_routes->end()) {
            uiEstRoute(to->name);
        } else {
            sendFnd(to->name);
        }
        return NULL;
    }
    
    /* since this is a connection, try now to find offline buddies */
    cur = bltop;
    while (cur) {
        if (!gaim_presence_is_online(cur->buddy->presence) && strncmp(cur->buddy->name, "con:", 4)) {
            sendFnd(cur->buddy->name); /* FIXME: this should not go on all connections */
        }
        cur = cur->next;
    }
    return NULL;
}

static void gp_addbuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group)
{
    char *route;
    
    /* there are special "buddies" that are connections: con:host */
    if (!strncmp(buddy->name, "con:", 4)) {
        if (!checkAutoConnect(buddy->name + 4)) {
            addAutoConnect(buddy->name + 4);
        }
        waitConn(buddy);
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
        
        /* also, the autofind list */
        if (!checkAutoFind(buddy->name)) {
            addAutoFind(buddy->name);
        }
        
        /* and try to find it */
        waitConn(buddy);
    }
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Buddy added.\n");
}

static void gp_removebuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group)
{
    /* remove the buddy from our buddy list */
    struct BuddyList *cur, *todel;
    
    if (bltop) {
        /* maybe it is the top! */
        if (!strcmp(bltop->buddy->name, buddy->name)) {
            /* remove it */
            todel = bltop;
            bltop = bltop->next;
            free(todel);
            return;
        }
        
        cur = bltop;
        while (cur->next) {
            if (!strcmp(cur->next->buddy->name, buddy->name)) {
                /* remove it */
                todel = cur->next;
                cur->next = cur->next->next;
                free(todel);
                return;
            }
            cur = cur->next;
        }
    }
    
    /* remove it from the auto* list */
    if (!strncmp(buddy->name, "con:", 4)) {
        remAutoConnect(buddy->name + 4);
    } else {
        remAutoFind(buddy->name);
    }
    
    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Removed buddy.\n");
}

static void gp_joinchat(GaimConnection *gc, GHashTable *data)
{

    char *name = g_hash_table_lookup(data, "room");
    if (name[0] == '#') {
        joinChat(name + 1);
        chat_join(regChat(name), name);
    } else {
        char *fullname = (char *) malloc(strlen(name) + 2);
        joinChat(name);
        sprintf(fullname, "#%s", name);
        chat_join(regChat(fullname), fullname);
        free(fullname);
    }

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Joined chat.\n");
}

static void gp_simplechatinvite() {}

static void gp_leavechat(GaimConnection *gc, int id)
{
    GaimConversation *gconv = gaim_find_chat(gc, id);
    leaveChat(gconv->name + 1);
    id = chatByName(gconv->name);

    free(chat_ids[id]);
    chat_ids[id] = NULL;

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Left chat.\n");
}

static int gp_saychat(GaimConnection *gc, int id, const char *msg, GaimMessageFlags flags)
{
    GaimConversation *gconv = gaim_find_chat(gc, id);
    char *omsg = strdup(msg);

    sendChat(gconv->name + 1, omsg);
    chat_msg(id, dn_name, gconv->name, omsg);
    
    free(omsg);

    gaim_debug(GAIM_DEBUG_INFO, "DirectNet", "Sent chat message.\n");

    return 1;
}

