/*
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

int handleUInput(char *inp);

int uiInit(char **envp)
{
    // This is the most basic UI
    int charin, ostrlen;
    char cmdbuf[32256];
    
    if (findGPG(envp) == -1) {
        printf("GPG was not found on your PATH!\n");
        return -1;
    }
    
    printf("What is your nick? ");
    fgets(dn_name, 1024, stdin);
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    printf("%s\n", gpgCreateKey());
    
    while (1) {
        printf("> ");
        fgets(cmdbuf, 32256, stdin);
        
        ostrlen = strlen(cmdbuf);
        if (cmdbuf[ostrlen-1] == '\n') {
            cmdbuf[ostrlen-1] = '\0';
        }
        
        if (handleUInput(cmdbuf)) {
            return 0;
        }
    }
    
    return 0;
}

int handleUInput(char *inp)
{
    char *params[50];
    int i, onparam;
    
    memset(params, 0, 50 * sizeof(char *));
    
    params[0] = inp;
    onparam = 1;
    
    for (i = 0; inp[i] != '\0'; i++) {
        if (inp[i] == ' ') {
            inp[i] = '\0';
            params[onparam] = inp+i+1;
            onparam++;
        }
    }
    
    if (!strncmp(params[0], "connect", 7)) {
        establishClient(params[1]);
    } else if (!strncmp(params[0], "say", 3)) {
        char *outparams[50], *route;
        
        memset(outparams, 0, 50 * sizeof(char *));
        
        // param 2- are all one
        for (i = 2; params[i+1] != NULL; i++) {
            params[i][strlen(params[i])] = ' ';
        }
        printf("A\n");
        route = hashSGet(dn_routes, params[1]);
        printf("A\n");
        outparams[0] = (char *) malloc((strlen(route)+1) * sizeof(char));
        printf("A\n");
        strcpy(outparams[0], route);
        printf("A\n");
        outparams[1] = dn_name;
        printf("A\n");
        outparams[2] = gpgTo(dn_name, params[1], params[2]);
        printf("A\n");
        handleRoutedMsg("msg", 1, 1, outparams);
        printf("A\n");
    } else if (!strncmp(params[0], "quit", 4)) {
        return 1;
    }
    
    return 0;
}

void uiDispMsg(char *from, char *msg)
{
    printf("%s: %s\n", from, msg);
}
