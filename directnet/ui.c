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

char *currentPartner;

int handleUInput(char *inp);

int uiInit(char **envp)
{
    // This is the most basic UI
    int charin, ostrlen;
    char cmdbuf[32256];
    
    // Always start by finding GPG
    if (findGPG(envp) == -1) {
        printf("GPG was not found on your PATH!\n");
        return -1;
    }
    
    // Then asking for the nick
    printf("What is your nick? ");
    fgets(dn_name, 1024, stdin);
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    // And creating the key
    printf("%s\n", gpgCreateKey());
    
    // You start in a conversation with nobody
    currentPartner = (char *) malloc(256 * sizeof(char));
    currentPartner[0] = '\0';
    
    while (1) {
        printf("%s> ", currentPartner);
        fflush(stdout);
        fgets(cmdbuf, 32256, stdin);
        
        ostrlen = strlen(cmdbuf);
        if (cmdbuf[ostrlen-1] == '\n') {
            cmdbuf[ostrlen-1] = '\0';
        }
        
        if (strlen(cmdbuf) != 0) {
            if (handleUInput(cmdbuf)) {
                free(currentPartner);
                return 0;
            }
        }
    }
    
    return 0;
}

int handleUInput(char *inp)
{
    // Is it a command?
    if (inp[0] == '/') {
        char *params[50];
        int i, onparam;
        
        // Tokenize the parameters by ' '
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
        
        
        if (!strncmp(params[0]+1, "c", 1)) {
            if (params[1] == NULL) {
                return 0;
            }
        
            // Connect to a given hostname
            establishClient(params[1]);
            return 0;
        } else if (!strncmp(params[0]+1, "f", 1)) {
            char outbuf[65536];
        
            if (params[1] == NULL) {
                return 0;
            }
        
            // Find a user by name
            buildCmd(outbuf, "fnd", 1, 1, "");
            addParam(outbuf, dn_name);
            addParam(outbuf, params[1]);
            addParam(outbuf, gpgExportKey());
        
            emitUnroutedMsg(-1, outbuf);
        } else if (!strncmp(params[0]+1, "t", 1)) {
            if (params[1] == NULL) {
                return 0;
            }
        
            strncpy(currentPartner, params[1], 256);
        } else if (!strncmp(params[0]+1, "q", 1)) {
            return 1;
        }
        
    } else {
        // Not a command, a message
        char *outparams[50], *route;
        
        if (currentPartner[0] == '\0') {
            printf("You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat.\n");
            fflush(stdout);
            return 0;
        }
        
        memset(outparams, 0, 50 * sizeof(char *));
        
        route = hashSGet(dn_routes, currentPartner);
        if (route == NULL) {
            printf("No route to user.\n");
            return 0;
        }
        outparams[0] = (char *) malloc((strlen(route)+1) * sizeof(char));
        strcpy(outparams[0], route);
        outparams[1] = dn_name;
        outparams[2] = gpgTo(dn_name, currentPartner, inp);
        handleRoutedMsg("msg", 1, 1, outparams);
        
        printf("to %s: %s\n", currentPartner, inp);
    }
    
    return 0;
}

void uiDispMsg(char *from, char *msg)
{
    printf("\n%s: %s\n%s> ", from, msg, currentPartner);
    fflush(stdout);
}
