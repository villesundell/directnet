/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "dnconfig.h"
#include "enc.h"
#include "lock.h"
#include "ui.h"

char *currentPartner;
char *crossinput;
int cinp, cinpd;

int handleUInput(const char *inp);
int handleAuto(char **params);

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
    
    charin = strlen(dn_name);
    if (dn_name[charin-1] == '\n') {
        dn_name[charin-1] = '\0';
    }
    
    // And creating the key
    encCreateKey();
    
    // dn_name_set is overloaded, it also means key is created
    dn_name_set = 1;
    
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

int handleUInput(const char *originp)
{
    char *inp = alloca(strlen(originp)+1);
    strcpy(inp, originp);
    
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
        } else if (params[0][1] == 'u') {
            // auto*
            handleAuto(params);
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

/* just received a /u, so handle auto-something */
int handleAuto(char **params)
{
    int i;
    
    switch (params[1][0]) {
        case 'c':
        case 'C':
            /* autoconnections ... */
            switch (params[2][0]) {
                case 'l':
                case 'L':
                    /* autoconnection list */
                    printf("Autoconnection list:\n");
                    dn_lock(&dn_ac_lock);
                    for (i = 0; i < DN_MAX_CONNS && dn_ac_list[i]; i++) {
                        printf("%s\n", dn_ac_list[i]);
                    }
                    dn_unlock(&dn_ac_lock);
                    break;
                    
                case 'a':
                case 'A':
                    /* autoconnection add */
                    if (!params[3]) {
                        printf("Use: /u c a <hostname>\n");
                        break;
                    }
                    addAutoConnect(params[3]);
                    break;
                    
                case 'r':
                case 'R':
                    /* autoconnection remove */
                    if (!params[3]) {
                        printf("Use: /u c r <hostname>\n");
                        break;
                    }
                    remAutoConnect(params[3]);
                    break;
                    
                default:
                    printf("Use: /u c {a|r} <hostname>\n");
                    break;
            }
            break;
            
        case 'f':
        case 'F':
            /* autofinds ... */
            switch (params[2][0]) {
                case 'l':
                case 'L':
                    /* autofind list */
                    printf("Autoconnection list:\n");
                    dn_lock(&dn_af_lock);
                    for (i = 0; i < DN_MAX_ROUTES && dn_af_list[i]; i++) {
                        printf("%s\n", dn_af_list[i]);
                    }
                    dn_unlock(&dn_af_lock);
                    break;
                    
                case 'a':
                case 'A':
                    /* autofind add */
                    if (!params[3]) {
                        printf("Use: /u f a <nick>\n");
                        break;
                    }
                    addAutoFind(params[3]);
                    break;
                    
                case 'r':
                case 'R':
                    /* autofind remove */
                    if (!params[3]) {
                        printf("Use: /u f r <nick>\n");
                        break;
                    }
                    remAutoFind(params[3]);
                    break;
                    
                default:
                    printf("Use: /u f {l|a|r} <nick>\n");
                    break;
            }
            break;
            
        default:
            printf("Use: /u {c|f} {l|a|r} {<hostname>|<nick>}\n"
                   "     c for autoconnections, f for autofinds\n"
                   "     l to list auto*\n"
                   "     a to add a hostname or nick\n"
                   "     r to remove a hostname or nick\n");
    }
}

void uiDispMsg(const char *from, const char *msg, const char *authmsg, int away)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s [%s]%s: %s\n%s> ", from, authmsg, away ? " [away]" : "", msg, currentPartner);
    fflush(stdout);
}

void uiAskAuthImport(const char *from, const char *msg, const char *sig)
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

void uiDispChatMsg(const char *chat, const char *from, const char *msg)
{
    while (!uiLoaded) sleep(0);
    printf("\n#%s: %s: %s\n%s> ", chat, from, msg, currentPartner);
    fflush(stdout);
}

void uiEstConn(const char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Connection established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiEstRoute(const char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Route established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseConn(const char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Connection lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseRoute(const char *from)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: Route lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiNoRoute(const char *to)
{
    while (!uiLoaded) sleep(0);
    printf("\n%s: No route to user.\n%s> ", to, currentPartner);
    fflush(stdout);
}
