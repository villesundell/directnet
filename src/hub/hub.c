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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "enc.h"
#include "ui.h"

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
    
    // The nick should be defined
    strcpy(dn_name, HUB_NAME);
    dn_name_set = 1;
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    // And create the key
    encCreateKey();
    
    // mark away
    setAway("This is a hub, there is no human reading your messages.");
    
    // OK, the UI is ready
    uiLoaded = 1;
    
    while (1) {
        /* just spin */
        sleep(32256);
    }
    
    return 0;
}

void uiDispMsg(const char *from, const char *msg, const char *authmsg, int away)
{
}

void uiAskAuthImport(const char *from, const char *msg, const char *sig)
{
}

void uiDispChatMsg(const char *chat, const char *from, const char *msg)
{
}

void uiEstConn(const char *from)
{
}

void uiEstRoute(const char *from)
{
}

void uiLoseConn(const char *from)
{
}

void uiLoseRoute(const char *from)
{
}

void uiNoRoute(const char *to)
{
}
