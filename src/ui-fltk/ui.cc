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
#include "ui.h"
}

#include <iostream>

#include "BuddyWindow.h"
#include "ChatWindow.h"

BuddyWindow *bw;
ChatWindow *cws[1024];

using namespace std;

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    /* get the name */
    strcpy(dn_name, "Gregor");
    
    /* blank our array of windows */
    memset(cws, 0, 1024 * sizeof(ChatWindow *));

    /* make the buddy window */
    bw = new BuddyWindow();
    bw->make_window();
    bw->buddyWindow->show();

    uiLoaded = 1;
    
    Fl::run();

    return 0;
}

ChatWindow *getWindow(const char *name)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (!strcmp(cws[i]->chatWindow->label(), name)) {
            cws[i]->chatWindow->show();
            return cws[i];
        }
    }
    
    cws[i] = new ChatWindow();
    cws[i]->make_window();
    cws[i]->chatWindow->label(strdup(name));
    cws[i]->chatWindow->show();
    
    return cws[i];
}

void estConn(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    w->value("");
    establishConnection(connTo);
    free(connTo);
}

void talkTo(Fl_Button *b, void *ignore)
{
    getWindow(bw->onlineList->text());
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
    
            cws[i]->textOut->insert(dispmsg);
            
            /* now actually send */
            sendMsg(to, msg);
            
            free(to);
            free(msg);
        }
    }
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    ChatWindow *cw;
    char *dispmsg;
    
    while (!uiLoaded) sleep(0);
    
    dispmsg = (char *) alloca((strlen(from) + strlen(msg) + 4) * sizeof(char));
    sprintf(dispmsg, "%s: %s\n", from, msg);
    
    cw = getWindow(from);
    
    cw->textOut->insert(dispmsg);
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
    cw->textOut->insert("Route established.\n");
}

extern "C" void uiLoseConn(char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(from);
    cw->textOut->insert("Connection lost.\n");
}

extern "C" void uiLoseRoute(char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(from);
    cw->textOut->insert("Route lost.\n");
}

extern "C" void uiNoRoute(char *to)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(to);
    cw->textOut->insert("You do not have a route to this user.\n");
}
