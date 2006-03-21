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
 */

#include <iostream>
#include <string>
using namespace std;

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
#include "ui.h"
#include "dn_event.h"

#include <assert.h>

#include <sys/time.h>
#include <event.h>

char *currentPartner;
char *crossinput;
int cinp, cinpd;

int hub = 0;
const char *hubname;

int handleUInput(const char *inp);
int handleAuto(char **params);

void resetInputPrompt();

int main(int argc, char ** argv, char **envp)
{
    // This is the most basic UI
    int charin, ostrlen, i;
    char cmdbuf[32256];

    event_init();

    // strip out --hub so dn_init won't explode
    // XXX: need better cmdline handling
    // boost?
    
    for (i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], "--hub")) {
            hub = 1;
            hubname = argv[i+1];
            memmove(&argv[i], &argv[i+2], sizeof argv[i] * (argc - i - 2));
            argc -= 2;
            break;
        }
    }
    
    dn_init(argc, argv);
    
    cinp = 0;
    cinpd = 0;
    crossinput = NULL;
    
    // Always start by finding encryption
    if (findEnc(envp) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return -1;
    }
    
    if (!hub) {
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
    } else { // hub mode
        authSetPW("", "");
        strcpy(dn_name, hubname);
    }
        
    
    // And creating the key
    encCreateKey();
    
    if (hub) {
        string away = "This is a hub, there is no human reading your messages.";
        setAway(&away);
    }
    
    // You start in a conversation with nobody
    currentPartner = (char *) malloc(DN_NAME_LEN * sizeof(char));
    currentPartner[0] = '\0';
    
    dn_goOnline();
    
    if (!hub) {
        printf("%s> ", currentPartner);
        fflush(stdout);

        resetInputPrompt();
    }

    event_loop(0);
    return 0;
}

#if 0
    while (1) {
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
#endif

dn_event_t input_ev;

char inputBuffer[65536];
int ib_pos = 0;

static void inputEvent(int cond, dn_event_t *ev);

void resetInputPrompt() {
    input_ev.event_type = DN_EV_FD;
    input_ev.event_info.fd.fd = 0;
    input_ev.event_info.fd.trigger = inputEvent;
    input_ev.event_info.fd.watch_cond = DN_EV_READ;
    dn_event_activate(&input_ev);
}

void inputEvent(int cond, dn_event_t *ev) {
    int i;
    int ret = read(0, inputBuffer + ib_pos, sizeof inputBuffer - ib_pos);
    if (ret <= 0) {
        perror ("console read error, shutting down");
        exit(1);
    }
    ib_pos += ret;
    while (ib_pos) {
        for (i = 0; i < ib_pos; i++) {
            if (inputBuffer[i] == '\n') {
                inputBuffer[i] = '\0';
                goto got_input;
            }
        }
        return;
got_input:
        handleUInput(inputBuffer);
        memmove(inputBuffer, inputBuffer + i + 1, ib_pos - i - 1);
        ib_pos -= i + 1;
    }
    if (!ib_pos) {
        printf("%s> ", currentPartner);
        fflush(stdout);
    }
}

int handleUInput(const char *originp)
{
    char *inp = (char *) alloca(strlen(originp)+1);
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
            string amsg = params[1];
            setAway(&amsg);
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
    set<string>::iterator aci, afi;
    
    switch (params[1][0]) {
        case 'c':
        case 'C':
            /* autoconnections ... */
            switch (params[2][0]) {
                case 'l':
                case 'L':
                    /* autoconnection list */
                    cout << "Autoconnection list:" << endl;
                    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
                        cout << *aci << endl;
                    }
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
                    cout << "Autofind list:" << endl;
                    for (afi = dn_af_list->begin(); aci != dn_af_list->end(); afi++) {
                        cout << *afi << endl;
                    }
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
    printf("\n%s [%s]%s: %s\n%s> ", from, authmsg, away ? " [away]" : "", msg, currentPartner);
    fflush(stdout);
}

void uiAskAuthImport(const char *from, const char *msg, const char *sig)
{
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
    printf("\n#%s: %s: %s\n%s> ", chat, from, msg, currentPartner);
    fflush(stdout);
}

void uiEstConn(const char *from)
{
    printf("\n%s: Connection established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiEstRoute(const char *from)
{
    printf("\n%s: Route established.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseConn(const char *from)
{
    printf("\n%s: Connection lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiLoseRoute(const char *from)
{
    printf("\n%s: Route lost.\n%s> ", from, currentPartner);
    fflush(stdout);
}

void uiNoRoute(const char *to)
{
    printf("\n%s: No route to user.\n%s> ", to, currentPartner);
    fflush(stdout);
}

struct dn_event_private {
    struct event cevt;
    dn_event_t *parent;
};

static void callback(int fd, short cond, void *payload) {
    dn_event_t *ev = (dn_event_t *) payload;
    if (ev->event_type == DN_EV_FD) {
        int c = 0;
        if (cond & EV_READ)
            c |= DN_EV_READ;
        if (cond & EV_WRITE)
            c |= DN_EV_WRITE;
        ev->event_info.fd.trigger(c, ev);
    } else if (ev->event_type = DN_EV_TIMER) {
        /* libevent clears the event for us in this case, clean up
         * the private data
         */
        free(ev->priv);
        ev->priv = NULL;
        ev->event_info.timer.trigger(ev);
    } else assert(0 == "bad event type");
}
    
#undef dn_event_activate

void dn_event_activate(dn_event_t *ev) {
    struct dn_event_private *priv = (struct dn_event_private *) malloc(sizeof *priv);
    priv->parent = ev;
    ev->priv = priv;
    switch (ev->event_type) {
        case DN_EV_FD:
            {
                int cond = EV_PERSIST;
                if (ev->event_info.fd.watch_cond & DN_EV_READ)
                    cond |= EV_READ;
                if (ev->event_info.fd.watch_cond & DN_EV_WRITE)
                    cond |= EV_WRITE;
                event_set(&priv->cevt, ev->event_info.fd.fd, cond, callback, ev);
                event_add(&priv->cevt, NULL);
                return;
            }
        case DN_EV_TIMER:
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                tv.tv_sec = ev->event_info.timer.t_sec - tv.tv_sec;
                tv.tv_usec = ev->event_info.timer.t_usec - tv.tv_usec;
                tv.tv_sec += tv.tv_usec / 1000000;
                tv.tv_usec %= 1000000;
                evtimer_set(&priv->cevt, callback, ev);
                event_add(&priv->cevt, &tv);
                return;
            }
        default:
            assert(0 == "bad event type");
    }
}

void dn_event_deactivate(dn_event_t *ev) {
    if (ev->priv == NULL) return;
    event_del(&ev->priv->cevt);
    free(ev->priv);
    ev->priv = NULL;
}
