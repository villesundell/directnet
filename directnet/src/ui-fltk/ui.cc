/*
 * Copyright 2005 Gregor Richards
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

extern "C" {
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "directnet.h"
#include "globals.h"
#include "ui.h"
}

#include <iostream>

#include "BuddyWindow.h"
#include "ChatWindow.h"
#include "NameWindow.h"

NameWindow *nw;
BuddyWindow *bw;
ChatWindow *cws[1024];

using namespace std;

static int uiQuit = 0;
ChatWindow *showcw = NULL;

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    /* blank our array of windows */
    memset(cws, 0, 1024 * sizeof(ChatWindow *));

    /* make the name window */
    nw = new NameWindow();
    nw->make_window();
    nw->nameWindow->show();
    
    /*Fl::run();*/
    while (!uiQuit) {
        if (showcw) {
            showcw->chatWindow->show();
            showcw = NULL;
        }
        Fl::wait(1);
    }

    return 0;
}

ChatWindow *getWindow(const char *name)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (!strcmp(cws[i]->chatWindow->label(), name)) {
            /*cws[i]->chatWindow->show();*/
            while (showcw) sleep(0);
            showcw = cws[i];
            return cws[i];
        }
    }
    
    cws[i] = new ChatWindow();
    cws[i]->make_window();
    cws[i]->chatWindow->label(strdup(name));
    /*cws[i]->chatWindow->show();*/
    while (showcw) sleep(0);
    showcw = cws[i];
    
    return cws[i];
}

void putOutput(ChatWindow *w, const char *txt)
{
    Fl_Text_Buffer *tb = w->textOut->buffer();
    
    w->textOut->insert_position(tb->length());
    w->textOut->insert(txt);
    w->textOut->show_insert_position();
    Fl::flush();
}

void setName(Fl_Input *w, void *ignore)
{
    strncpy(dn_name, w->value(), DN_NAME_LEN);
    nw->nameWindow->hide();
    
    /* make the buddy window */
    bw = new BuddyWindow();
    bw->make_window();
    bw->buddyWindow->show();

    uiLoaded = 1;
}

void mainWinClosed(Fl_Double_Window *w, void *ignore)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        cws[i]->chatWindow->hide();
    }
    
    w->hide();
    
    uiQuit = 1;
}

void estConn(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    w->value("");
    establishConnection(connTo);
    free(connTo);
}

void estFnd(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    w->value("");
    sendFnd(connTo);
    free(connTo);
}

void estCht(Fl_Input *w, void *ignore)
{
    char chatOn[strlen(w->value()) + 2];
    
    chatOn[0] = '#';
    strcpy(chatOn + 1, w->value());
    w->value("");

    joinChat(chatOn + 1);
    getWindow(chatOn);
}

void closeChat(Fl_Double_Window *w, void *ignore)
{
    char *name;
    
    name = strdup(w->label());
    
    // if it's a chat window, we should leave the chat now
    if (name[0] == '#') {
        leaveChat(name + 1);
    }
    
    w->hide();
    
    free(name);
}

void talkTo(Fl_Button *b, void *ignore)
{
    int sel;
    
    sel = bw->onlineList->value();
    if (sel != 0) {
        getWindow(bw->onlineList->text(sel));
    }
}

void sendInput(Fl_Input *w, void *ignore)
{
    /* sad way to figure out what window we're in ... */
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (cws[i]->textIn == w) {
            /* this is our window */
            char *dispmsg, *to, *msg;
            
            to = strdup(cws[i]->chatWindow->label());
            
            msg = strdup(cws[i]->textIn->value());
            cws[i]->textIn->value("");
    
            dispmsg = (char *) alloca((strlen(dn_name) + strlen(msg) + 4) * sizeof(char));
            sprintf(dispmsg, "%s: %s\n", dn_name, msg);

            if (to[0] == '#') {
                // Chat room
                sendChat(to + 1, msg);
                putOutput(cws[i], dispmsg);
            } else {
                if (sendMsg(to, msg)) {
                    putOutput(cws[i], dispmsg);
                }
            }
            
            free(to);
            free(msg);
            
            return;
        }
    }
}

void flDispMsg(char *window, char *from, char *msg)
{
    ChatWindow *cw;
    char *dispmsg;
    
    while (!uiLoaded) sleep(0);
    
    dispmsg = (char *) alloca((strlen(from) + strlen(msg) + 4) * sizeof(char));
    sprintf(dispmsg, "%s: %s\n", from, msg);
    
    cw = getWindow(window);
    
    putOutput(cw, dispmsg);
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    flDispMsg(from, from, msg);
}

extern "C" void uiDispChatMsg(char *chat, char *from, char *msg)
{
    char chatWHash[strlen(chat) + 2];
    
    chatWHash[0] = '#';
    strcpy(chatWHash + 1, chat);
    
    flDispMsg(chatWHash, from, msg);
}

extern "C" void uiEstConn(char *from)
{
    /* what to use here...? */
}

extern "C" void uiEstRoute(char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    bw->onlineList->add(from);
    
    cw = getWindow(from);
    putOutput(cw, "Route established.\n");
}

void removeFromList(char *name)
{
    int i;
    
    for (i = 1; bw->onlineList->text(i) != NULL; i++) {
        if (!strcmp(bw->onlineList->text(i), name)) {
            bw->onlineList->remove(i);
            i--;
        }
    }
}

extern "C" void uiLoseConn(char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    removeFromList(from);
    
    cw = getWindow(from);
    putOutput(cw, "Connection lost.\n");
}

extern "C" void uiLoseRoute(char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    removeFromList(from);
    
    cw = getWindow(from);
    putOutput(cw, "Route lost.\n");
}

extern "C" void uiNoRoute(char *to)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(to);
    putOutput(cw, "You do not have a route to this user.\n");
}
