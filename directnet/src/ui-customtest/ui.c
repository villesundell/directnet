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

#include "auth.h"
#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "enc.h"
#include "ui.h"

char *currentPartner;
char *crossinput;
int cinp, cinpd;

int handleUInput(char *inp);

int uiInit(int argc, char ** argv, char **envp)
{
    // This is the most basic UI
    int charin, ostrlen;
    char cmdbuf[32256];
    
    cinp = 0;
    cinpd = 0;
    crossinput = NULL;
    
    // Always start by finding encryption
    if (findEnc(envp) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return -1;
    }
    
#ifndef __WIN32
    /* Then authentication */
    if (!authInit()) {
        printf("Authentication failed to initialize!\n");
        return -1;
    } else if (authNeedPW()) {
        char *nm, *pswd;
        int osl;
        
        /* name */
        nm = (char *) malloc(256 * sizeof(char));
        printf("%s: ", authUsername);
        fflush(stdout);
        nm[255] = '\0';
        fgets(nm, 255, stdin);
        osl = strlen(nm) - 1;
        if (nm[osl] == '\n') nm[osl] = '\0';
        
        /* don't input the pass if the UN was 0-length */
        if (nm[0]) {
            /* password */
            pswd = (char *) malloc(256 * sizeof(char));
            /* lame way to cover it */
            printf("%s: \x1b[30;40m", authPW);
            fflush(stdout);
            pswd[255] = '\0';
            fgets(pswd, 255, stdin);
            osl = strlen(pswd) - 1;
            if (pswd[osl] == '\n') pswd[osl] = '\0';
            printf("\x1b[0m");
            fflush(stdout);
        
            authSetPW(nm, pswd);
            free(pswd);
        } else {
            authSetPW(nm, "");
        }
        
        free(nm);
    }
#else
    authSetPW("", "");
#endif
    
    // Then asking for the nick
    printf("What is your nick? ");
    fgets(dn_name, DN_NAME_LEN, stdin);
    dn_name[DN_NAME_LEN] = '\0';
    dn_name_set = 1;
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    // And creating the key
    printf("%s\n", encCreateKey());
    
    // You start in a conversation with nobody
    currentPartner = (char *) malloc(DN_NAME_LEN * sizeof(char));
    currentPartner[0] = '\0';
    
    // OK, the UI is ready
    uiLoaded = 1;
    
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
    // Is it crossinput?
    if (cinp) {
        cinp = 0;
        crossinput = strdup(inp);
        cinpd = 1;
        return 0;
    }
    
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
        
        if (params[0][1] == 'a') {
            setAway(params[1]);
            if (params[1]) {
                printf("\nAway message set.\n");
            } else {
                printf("\nAway message unset.\n");
            }
            return 0;
        } else if (params[0][1] == 'c') {
            if (params[1] == NULL) {
                return 0;
            }
        
            // Connect to a given hostname or user
            establishConnection(params[1]);
            return 0;
        } else if (params[0][1] == 'f') {
            if (params[1] == NULL) {
                return 0;
            }
            
            sendFnd(params[1]);
        } else if (params[0][1] == 'k') {
            if (currentPartner[0] == '\0') {
                printf("You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat.\n");
                fflush(stdout);
                return 0;
            }
            if (currentPartner[0] == '#') return 0;
            
            sendAuthKey(currentPartner);
        } else if (params[0][1] == 't') {
            if (params[1] == NULL) {
                return 0;
            }
        
            strncpy(currentPartner, params[1], DN_NAME_LEN);
            
            if (currentPartner[0] == '#') {
                // Join the chat
                joinChat(currentPartner+1);
            }
        } else if (params[0][1] == 'q') {
            return 1;
        }
        
    } else {
        // Not a command, a message
        if (currentPartner[0] == '\0') {
            printf("You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat.\n");
            fflush(stdout);
            return 0;
        }
        
        // Is it a chat?
        if (currentPartner[0] == '#') {
            sendChat(currentPartner+1, inp);
        } else {
            if (sendMsg(currentPartner, inp)) {
                printf("to %s: %s\n", currentPartner, inp);
            }
        }
    }
    
    return 0;
}

void uiDispMsg(char *from, char *msg, char *authmsg, int away)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s [%s]%s: %s\n%s> ", from, authmsg, away ? " [away]" : "", msg, currentPartner);
    fflush(stdout);
}

void uiAskAuthImport(char *from, char *msg, char *sig)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s has asked you to import the key '%s'.  Do you accept?\n? ", from, sig);
    fflush(stdout);
    
    cinpd = 0;
    cinp = 1;
    while (!cinpd) sleep(0);
    
    if (crossinput[0] == 'y' || crossinput[0] == 'Y') {
        printf("\nImporting %s ...\n%s> ", sig, currentPartner);
        fflush(stdout);
        authImport(msg);
    }
    free(crossinput);
}

void uiDispChatMsg(char *chat, char *from, char *msg)
{
    while (!uiLoaded) sleep(0);
    printf("\n#%s: %s: %s\n%s> ", chat, from, msg, currentPartner);
    fflush(stdout);
}

void uiEstConn(char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Connection established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiEstRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Route established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseConn(char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Connection lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Route lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiNoRoute(char *to)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: No route to user.\n%s> ", to, currentPartner);
    fflush(stdout);
}
