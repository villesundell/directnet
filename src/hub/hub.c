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
#include "gpg.h"
#include "ui.h"

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
    
    // The nick should be defined
    strcpy(dn_name, HUB_NAME);
    dn_name_set = 1;
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    // And create the key
    gpgCreateKey();
    
    // OK, the UI is ready
    uiLoaded = 1;
    
    // Make initial connections from ~/.dnhub
    homedir = getenv("HOME");
    if (homedir != NULL) {
        FILE *dnhf;
        char line[1024];
        int ostrlen;
        
        dnhomefile = (char *) malloc((strlen(homedir) + 8) * sizeof(char));
        sprintf(dnhomefile, "%s/.dnhub", homedir);
        
        dnhf = fopen(dnhomefile, "r");
        if (dnhf) {
            while (!feof(dnhf)) {
                fgets(line, 1024, dnhf);
                
                ostrlen = strlen(line);
                if (line[ostrlen-1] == '\n') {
                    line[ostrlen-1] = '\0';
                }
                printf("Connecting to %s\n", line);
                establishConnection(line);
            }
            
            fclose(dnhf);
        }
        free(dnhomefile);
    }
    
    while (1) {
        /* just spin */
        sleep(32256);
    }
    
    return 0;
}

void uiDispMsg(char *from, char *msg)
{
    while (!uiLoaded) sleep(0);
    /* just respond that this is a hub */
    sendMsg(from, "This is a hub, and cannot respond.");
}

void uiDispChatMsg(char *chat, char *from, char *msg)
{
}

void uiEstConn(char *from)
{
}

void uiEstRoute(char *from)
{
}

void uiLoseConn(char *from)
{
}

void uiLoseRoute(char *from)
{
}

void uiNoRoute(char *to)
{
}
