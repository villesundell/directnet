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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "directnet.h"
#include "ui.h"
}

#include "ui_fl.h"

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    /* get the name */
    strcpy(dn_name, "Gregor");
    
    Fl_Double_Window *w;
    pthread_t subpthread;
    pthread_attr_t ptattr;
    Fl_Text_Buffer *tb;
    
    w = ui_fl();

    tb = new Fl_Text_Buffer(32256);
    textOut->buffer(tb);
    w->show();
    
    uiLoaded = 1;
    
    Fl::run();

    return 0;
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(from) + strlen(msg) + 4) * sizeof(char));
    sprintf(outmsg, "%s: %s\n", from, msg);
    
    textOut->insert(outmsg);
}

extern "C" void uiEstConn(char *from)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(from) + 44) * sizeof(char));
    sprintf(outmsg, "A connection has been established with %s.\n", from);
    
    textOut->insert(outmsg);
}

extern "C" void uiEstRoute(char *from)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(from) + 39) * sizeof(char));
    sprintf(outmsg, "A route has been established with %s.\n", from);
    
    textOut->insert(outmsg);
}

extern "C" void uiLoseConn(char *from)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(from) + 37) * sizeof(char));
    sprintf(outmsg, "The connection with %s has been lost.\n", from);
    
    textOut->insert(outmsg);
}

extern "C" void uiLoseRoute(char *from)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(from) + 32) * sizeof(char));
    sprintf(outmsg, "The route with %s has been lost.\n", from);
    
    textOut->insert(outmsg);
}

extern "C" void uiNoRoute(char *to)
{
    char *outmsg;
    
    while (!uiLoaded) sleep(0);
    
    outmsg = (char *) alloca((strlen(to) + 30) * sizeof(char));
    sprintf(outmsg, "You do not have a route to %s.\n", to);
    
    textOut->insert(outmsg);
}
