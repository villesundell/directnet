#ifndef DN_GAIM_PLUGIN_H
#define DN_GAIM_PLUGIN_H

#include <internal.h>
#include <prpl.h>
#include <debug.h>
#include <version.h>
#include <conversation.h>

static const char *gp_listicon(GaimAccount *a, GaimBuddy *b);
static GList *gp_chatinfo(GaimConnection *gc);
GHashTable *gp_chatinfo_defaults(GaimConnection *gc, const char *chat_name);
static GList *gp_awaystates(GaimConnection *gc);
static void gp_login(GaimAccount *account);
static int gp_sendim(GaimConnection *gc, const char *who, const char *message, GaimConvImFlags flags);
static void gp_setaway(GaimConnection *gc, const char *state, const char *text);
static void gp_addbuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group);
static void gp_removebuddy(GaimConnection *gc, GaimBuddy *buddy, GaimGroup *group);
static void gp_joinchat(GaimConnection *gc, GHashTable *data);
static void gp_simplechatinvite();
static void gp_leavechat(GaimConnection *gc, int id);
static int gp_saychat(GaimConnection *gc, int id, const char *msg);

#endif
