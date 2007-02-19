/*
 * Copyright 2004, 2005, 2006, 2007  Gregor Richards
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

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "chat.h"
#include "client.h"
#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dnconfig.h"
#include "enc.h"
#include "ui.h"
#include "dn_event.h"

#include <assert.h>

#include <sys/time.h>
#include <event.h>

string currentPartner;
void (*crossinput)(const string &inp);
string *csig, *cmsg;
bool cinp = false;

bool hub = false;
const char *hubname;

void handleUInput(const string &inp);
void handleAuto(vector<string> &params);

void resetInputPrompt();

int main(int argc, char **argv, char **envp)
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
    
    // Always start by finding encryption
    if (findEnc(envp) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return -1;
    }
    
    if (!hub) {
#if 0 // authentication unsupported right now
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
    currentPartner = "";
    
    dn_goOnline();
    
    if (!hub) {
        cout << "> ";
        cout.flush();
        
        resetInputPrompt();
    }
    
    event_loop(0);
    return 0;
}

char inputBuffer[65536];
int ib_pos = 0;

static void inputEvent(dn_event_fd *ev, int cond);
dn_event_fd input_ev(0, DN_EV_READ, inputEvent, NULL);

void resetInputPrompt() {
    input_ev.activate();
}

void inputEvent(dn_event_fd *ev, int cond) {
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
        cout << currentPartner << "> ";
        cout.flush();
    }
}

void handleUInput(const string &inp)
{
    // Is it crossinput?
    if (cinp) {
        crossinput(inp);
        return;
    }
    
    // Is it a command?
    if (inp[0] == '/') {
        vector<string> params;
        int x, y;
        
        // Tokenize the parameters by ' '
        for (x = 0; x < inp.length(); x = y + 1) {
            y = inp.find_first_of(' ', x);
            if (y == string::npos) {
                y = inp.length();
            }
            params.push_back(inp.substr(x, y - x));
        }

        if (params[0] == "/away" || params[0] == "/a") {
            if (params.size() > 1) {
                setAway(&params[1]);
                cout << "Away message set." << endl;
            } else {
                setAway(NULL);
                cout << "Away message unset." << endl;
            }
            return;
        } else if (params[0] == "/connect" || params[0] == "/c") {
            if (params.size() <= 1) {
                return;
            }
            
            // Connect to a given hostname or user
            establishConnection(params[1]);
            return;
        } else if (params[0] == "/find" || params[0] == "/f") {
            if (params.size() <= 1) {
                return;
            }
            
            sendFnd(params[1]);
        } else if (params[0] == "/key" || params[0] == "/k") {
            if (currentPartner == "") {
                cout << "You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat." << endl;
                return;
            }
            if (currentPartner[0] == '#') return;
            
            sendAuthKey(currentPartner);
        } else if (params[0] == "/talk" || params[0] == "/t") {
            if (params.size() <= 1) {
                return;
            }
            
            currentPartner = params[1];
            
            if (currentPartner[0] == '#') {
                // Join the chat
                // joinChat(currentPartner.substr(1)); FIXME
            }
        } else if (params[0] == "/auto") {
            // auto*
            handleAuto(params);
        } else if (params[0] == "/quit" || params[0] == "/q") {
            exit(0);
        }
        
    } else {
        if (inp.length() == 0) return;
        
        // Not a command, a message
        if (currentPartner == "") {
            cout << "You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat." << endl;
            return;
        }
        
        // Is it a chat?
        if (currentPartner[0] == '#') {
            sendChat(currentPartner, inp);
        } else {
            if (sendMsg(currentPartner, inp)) {
                cout << "to " << currentPartner << ": " << inp << endl;
            }
        }
    }
    
    return;
}

/* just received a /u, so handle auto-something */
void handleAuto(vector<string> &params)
{
    int i;
    set<string>::iterator aci, afi;
    
#define AUTO_USE cout << "Use: /auto {c|n} {l|a|r} <hostname|nick>" << endl
    if (params.size() <= 2) {
        AUTO_USE;
        return;
    }
    
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
                    if (params.size() <= 3) {
                        AUTO_USE;
                        break;
                    }
                    addAutoConnect(params[3]);
                    break;
                    
                case 'r':
                case 'R':
                    /* autoconnection remove */
                    if (params.size() <= 3) {
                        AUTO_USE;
                        break;
                    }
                    remAutoConnect(params[3]);
                    break;
                    
                default:
                    AUTO_USE;
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
                    if (params.size() <= 3) {
                        AUTO_USE;
                        break;
                    }
                    addAutoFind(params[3]);
                    break;
                    
                case 'r':
                case 'R':
                    /* autofind remove */
                    if (params.size() <= 3) {
                        AUTO_USE;
                        break;
                    }
                    remAutoFind(params[3]);
                    break;
                    
                default:
                    AUTO_USE;
                    break;
            }
            break;
            
        default:
            cout << "Use: /auto {c|f} {l|a|r} {<hostname>|<nick>}" << endl
            << "     c for autoconnections, f for autofinds" << endl
            << "     l to list auto*" << endl
            << "     a to add a hostname or nick" << endl
            << "     r to remove a hostname or nick" << endl;
    }
}

void uiDispMsg(const string &from, const string &msg, const string &authmsg, int away)
{
    if (!hub) {
        cout << endl << from << " [" << authmsg << "]" << (away ? " [away" : "") <<
        ": " << msg << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiAskAuthImport2(const string &acpt);

void uiAskAuthImport(const string &from, const string &msg, const string &sig)
{
    if (!hub) {
        cout << endl << from << " has asked you to import the key '" << sig <<
        "'.  Do you accept?" << endl << "? ";
        cout.flush();
        
        crossinput = uiAskAuthImport2;
        csig = new string(sig);
        cmsg = new string(msg);
        cinp = true;
    }
}

void uiAskAuthImport2(const string &acpt)
{
    if (!hub) {
        if (acpt[0] == 'y' || acpt[0] == 'Y') {
            cout << endl << "Importing " << *csig << " ..." << endl;
            authImport(cmsg->c_str());
        }
        delete csig;
        delete cmsg;
        cinp = false;
    }
}

void uiDispChatMsg(const string &chat, const string &from, const string &msg)
{
    if (!hub) {
        cout << endl << "#" << chat << ": " << from << ": " << msg << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiEstConn(const string &from)
{
    if (!hub) {
        cout << endl << from << ": Connection established." << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiEstRoute(const string &from)
{
    if (!hub) {
        cout << endl << from << ": Route established." << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiLoseConn(const string &from)
{
    if (!hub) {
        cout << endl << from << ": Connection lost." << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiLoseRoute(const string &from)
{
    if (!hub) {
        cout << endl << from << ": Route lost." << endl << currentPartner << "> ";
        cout.flush();
    }
}

void uiNoRoute(const string &to)
{
    if (!hub) {
        cout << endl << to << ": No route to user." << endl << currentPartner << "> ";
        cout.flush();
    }
}

class dn_event_private {
    public:
    struct event cevt;
    dn_event *parent;
};

class dn_event_access {
    public:

        static void callback(int fd, short cond, void *payload) {
            dn_event *ev = (dn_event *) payload;
            dn_event_fd *fde = dynamic_cast<dn_event_fd *>(ev);
            dn_event_timer *timer = dynamic_cast<dn_event_timer *>(ev);

            if (fde) {
                int c = 0;
                if (cond & EV_READ)
                    c |= DN_EV_READ;
                if (cond & EV_WRITE)
                    c |= DN_EV_WRITE;
                fde->trigger(fde, c);
            } else if (timer) {
                /* libevent clears the event for us in this case, clean up
                 * the private data
                 */
                delete timer->priv;
                timer->priv = NULL;
                timer->trigger(timer);
            } else assert(0 == "bad event type");
        }
};

void dn_event_fd::activate() {

    assert(!is_active);
    
    priv = new dn_event_private();
    priv->parent = this;

    int cond = EV_PERSIST;
    if (this->cond & DN_EV_READ)
        cond |= EV_READ;
    if (this->cond & DN_EV_WRITE)
        cond |= EV_WRITE;
    event_set(&priv->cevt, this->fd, cond, dn_event_access::callback, static_cast<dn_event *>(this));
    event_add(&priv->cevt, NULL);

    is_active = true;
}

void dn_event_timer::activate() {
    assert(!is_active);

    priv = new dn_event_private();
    priv->parent = this;
        
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tv.tv_sec = this->tv.tv_sec - tv.tv_sec;
    tv.tv_usec = this->tv.tv_usec - tv.tv_usec;
    tv.tv_sec += tv.tv_usec / 1000000;
    tv.tv_usec %= 1000000;
    evtimer_set(&priv->cevt, dn_event_access::callback, static_cast<dn_event *>(this));
    event_add(&priv->cevt, &tv);
    
    is_active = true;
}

void dn_event_fd::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    event_del(&priv->cevt);
    delete priv;
    priv = NULL;
}

void dn_event_timer::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    event_del(&priv->cevt);
    delete priv;
    priv = NULL;
}
