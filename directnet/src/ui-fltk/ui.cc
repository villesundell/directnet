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
#include "ChatWindow.h

ChatWindow *cws[1024];

using namespace std;

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    BuddyWindow *w;
    
    /* get the name */
    strcpy(dn_name, "Gregor");
    
    /* blank our array of windows */
    memset(cws, 0, 1024 * sizeof(ChatWindow *));

    /* make the buddy window */
    w = new BuddyWindow();
    w->make_window();
    w->buddyWindow->show();

    uiLoaded = 1;
    
    Fl::run();

    return 0;
}

ChatWindow *getWindow(char *name)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (!strcmp(cws[i]->textOut->label(), name)) {
            cws[i]->show();
            return cws[i];
        }
    }
    
    cws[i] = new ChatWindow();
    cws[i]->make_window();
    cws[i]->chatWindow->label(name);
    cws[i]->textOut->label(name);
    cws[i]->show();
    
    return cws[i];
}

void estConn(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    establishConnection(connTo);
    free(connTo);
}

void sendInput(Fl_Input *w, void *ignore)
{
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    ChatWindow *cw;
    char *dispmsg;
    
    while (!uiLoaded) sleep(0);
    
    dispmsg = alloca((strlen(from) + strlen(msg) + 4) * sizeof(char));
    sprintf(dispmsg, "%s: %s\n", from, msg);
    
    cw = getWindow(from);
    
    cw->textOut->insert(dispmsg);
}

extern "C" void uiEstConn(char *from)
{
    while (!uiLoaded) sleep(0);
}

extern "C" void uiEstRoute(char *from)
{
    while (!uiLoaded) sleep(0);
}

extern "C" void uiLoseConn(char *from)
{
    while (!uiLoaded) sleep(0);
}

extern "C" void uiLoseRoute(char *from)
{
    while (!uiLoaded) sleep(0);
}

extern "C" void uiNoRoute(char *to)
{
    while (!uiLoaded) sleep(0);
}
