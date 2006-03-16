/*
 * Copyright 2005, 2006  Gregor Richards
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

#include "auth.h"
#include "connection.h"
#include "directnet.h"
#include "dnconfig.h"
#include "globals.h"
#include "enc.h"
#include "lock.h"
#include "ui.h"
}

#include <iostream>

#include "FL/fl_ask.H"

#include "AutoConnWindow.h"
#include "AwayWindow.h"
#include "BuddyWindow.h"
#include "ChatWindow.h"
#include "NameWindow.h"

#ifdef WIN32
#define sleep _sleep
#include <malloc.h>
#endif

NameWindow *nw;
BuddyWindow *bw;
ChatWindow *cws[1024];
AutoConnWindow *acw;

bool wantLock;
DN_LOCK wantLock_lock;

// keep track of where the buddies end and the connection list begins in onlineLis
int olConnsLoc;

using namespace std;

static int uiQuit = 0;
ChatWindow *showcw = NULL;

int flt1_ask(const char *msg, int t1)
{
    static char *umsg = NULL;
    static int theret = 0;
    
    if (t1) {
        /* I'm thread one, just try to display */
        if (!umsg) return 0;
        Fl::flush();
        theret = fl_ask(umsg);
        free(umsg);
        umsg = NULL;
        return 0;
    } else {
        /* I'm not thread one, wait for the answer */
        while (umsg) sleep(0);
        umsg = strdup(msg);
        while (umsg) sleep(0);
        return theret;
    }
}

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    /* Always start by finding encryption */
    if (findEnc(envp) == -1) {
        printf("No encryption binaries were found on your PATH!\n");
        return -1;
    }
    
#ifndef __WIN32
    /* Then authentication */
    if (!authInit()) {
        printf("Authentication failed to initialize!\n");
        return -1;
    } else if (authNeedPW()) {
        char *nm, *pswd;
        const char *cnm, *cpswd;
        
        /* name */
        cnm = fl_input(authUsername, NULL);
        if (cnm) nm = strdup(cnm);
        else nm = strdup("");
        
        /* only if we got a name do we get a pass */
        if (nm[0]) {
            /* password */
            cpswd = fl_password(authPW, NULL);
            if (cpswd) pswd = strdup(cpswd);
            else pswd = strdup("");
        } else {
            pswd = strdup("");
        }
        
        authSetPW(nm, pswd);
        free(nm);
        free(pswd);
    }
#else
    authSetPW("", "");
#endif

    /* And creating the key */
    encCreateKey();
    
    /* wantlock allows other threads to get the display lock */
    wantLock = false;
    dn_lockInit(&wantLock_lock);
    
    /* blank our array of windows */
    memset(cws, 0, 1024 * sizeof(ChatWindow *));

    /* make the name window */
    nw = new NameWindow();
    nw->make_window();
    nw->nameWindow->show();
    
    /*Fl::run();*/
    while (!uiQuit) {
        while (wantLock) sleep(0);
        
        if (showcw) {
            dn_lock(&displayLock);
            showcw->chatWindow->show();
            showcw = NULL;
            dn_unlock(&displayLock);
        }
        
        /* check for questions */
        flt1_ask(NULL, 1);
        
        dn_lock(&displayLock);
        Fl::wait(1);
        dn_unlock(&displayLock);
    }

    return 0;
}

void wantDisplayLock()
{
    while (true) {
        if (!wantLock) {
            dn_lock(&wantLock_lock);
            if (wantLock) {
                dn_unlock(&wantLock_lock);
                continue;
            }
            wantLock = true;
            dn_unlock(&wantLock_lock);
            break;
        }
        sleep(0);
    }
}

ChatWindow *getWindow(const char *name)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (!strcmp(cws[i]->chatWindow->label(), name)) {
            /*cws[i]->chatWindow->show();*/
            dn_unlock(&displayLock);
            while (showcw) sleep(0);
            wantDisplayLock();
            dn_lock(&displayLock);
            wantLock = false;
            showcw = cws[i];
            return cws[i];
        }
    }
    
    cws[i] = new ChatWindow();
    cws[i]->make_window();
    cws[i]->chatWindow->label(strdup(name));
    
    if (checkAutoFind(name)) {
        cws[i]->bAutoFind->label("Remove from Autofind List");
    } else {
        cws[i]->bAutoFind->label("Add to Autofind List");
    }
    
    /*cws[i]->chatWindow->show();*/
    dn_unlock(&displayLock);
    while (showcw) sleep(0);
    wantDisplayLock();
    dn_lock(&displayLock);
    wantLock = false;
    showcw = cws[i];
    
    return cws[i];
}

void putOutput(ChatWindow *w, const char *txt)
{
    Fl_Text_Buffer *tb = w->textOut->buffer();
    
    w->textOut->insert_position(tb->length());
    w->textOut->insert(txt);
    w->textOut->show_insert_position();
    Fl::flush();
}

void setName(Fl_Input *w, void *ignore)
{
    int i;
    
    strncpy(dn_name, w->value(), DN_NAME_LEN);
    dn_name[DN_NAME_LEN] = '\0';
    dn_name_set = 1;
    nw->nameWindow->hide();
    
    /* make the buddy window */
    bw = new BuddyWindow();
    bw->make_window();
    bw->buddyWindow->show();
    
    /* prep the onlineList */
    bw->onlineList->add("@c@mBuddies");
    
    /* add the autofind list */
    dn_lock(&dn_af_lock);
    for (i = 0; i < DN_MAX_ROUTES && dn_af_list[i]; i++) {
        char *toadd = (char *) malloc(strlen(dn_af_list[i]) + 3);
        if (!toadd) { perror("malloc"); exit(1); }
        sprintf(toadd, "@i%s", dn_af_list[i]);
        bw->onlineList->add(toadd);
        free(toadd);
    }
    dn_unlock(&dn_af_lock);
    
    /* add the autoconnect list */
    bw->onlineList->add("@c@mAutomatic Connections");
    olConnsLoc = bw->onlineList->size();
    dn_lock(&dn_ac_lock);
    for (i = 0; i < DN_MAX_ROUTES && dn_ac_list[i]; i++) {
        char *toadd = (char *) malloc(strlen(dn_ac_list[i]) + 3);
        if (!toadd) { perror("malloc"); exit(1); }
        sprintf(toadd, "%s", dn_ac_list[i]);
        bw->onlineList->add(toadd);
        free(toadd);
    }
    dn_unlock(&dn_ac_lock);
    
    /* finally, the option to add new autoconnections */
    bw->onlineList->add("@c@iAdd new autoconnection");
    
    uiLoaded = 1;
}

void mainWinClosed(Fl_Double_Window *w, void *ignore)
{
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        cws[i]->chatWindow->hide();
    }
    
    w->hide();
    
    uiQuit = 1;
}

void estConn(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    w->value("");
    establishConnection(connTo);
    free(connTo);
}

void estFnd(Fl_Input *w, void *ignore)
{
    char *connTo;
    
    connTo = strdup(w->value());
    w->value("");
    sendFnd(connTo);
    free(connTo);
}

void estCht(Fl_Input *w, void *ignore)
{
    char chatOn[strlen(w->value()) + 2];
    
    if (*w->value() == '#') {
        strcpy(chatOn, w->value());
    } else {
        chatOn[0] = '#';
        strcpy(chatOn + 1, w->value());
    }
    
    w->value("");

    joinChat(chatOn + 1);
    getWindow(chatOn);
}

void closeChat(Fl_Double_Window *w, void *ignore)
{
    char *name;
    
    name = strdup(w->label());
    
    // if it's a chat window, we should leave the chat now
    if (name[0] == '#') {
        leaveChat(name + 1);
    }
    
    w->hide();
    
    free(name);
}

void flSelectBuddy(Fl_Browser *flb, void *ignore)
{
    int i = flb->value();

    // ignore selection of the headers
    if (i == 1 || i == olConnsLoc) {
        flb->deselect();
        return;
    }
    
    // change the chat button depending on where in the list we are
    if (i < olConnsLoc) {
        bw->chatButton->label("Chat");
    } else if (i < flb->size()) {
        bw->chatButton->label("Remove from Autoconnection List");
    } else {
        bw->chatButton->label("Add new Autoconnection");
    }
}

void talkTo(Fl_Button *b, void *ignore)
{
    int sel;
    
    sel = bw->onlineList->value();
    if (sel == 0) return;
    
    // this could either be a chat ...
    if (sel > 1 && sel < olConnsLoc) {
        getWindow(bw->onlineList->text(sel) + 2);
    }
    
    // or a removal
    else if (sel > olConnsLoc && sel < bw->onlineList->size()) {
        remAutoConnect(bw->onlineList->text(sel));
        bw->onlineList->remove(sel);
    }
    
    // or an addition
    else {
        if (!acw) {
            acw = new AutoConnWindow();
            acw->make_window();
            acw->autoConnWindow->show();
        } else {
            acw->hostname->value("");
            acw->autoConnWindow->show();
        }
    }
}

void sendInput(Fl_Input *w, void *ignore)
{
    /* sad way to figure out what window we're in ... */
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (cws[i]->textIn == w) {
            /* this is our window */
            char *dispmsg, *to, *msg;
            
            to = strdup(cws[i]->chatWindow->label());
            
            msg = strdup(cws[i]->textIn->value());
            cws[i]->textIn->value("");
    
            dispmsg = (char *) alloca((strlen(dn_name) + strlen(msg) + 4) * sizeof(char));
            sprintf(dispmsg, "%s: %s\n", dn_name, msg);

            if (to[0] == '#') {
                // Chat room
                sendChat(to + 1, msg);
                putOutput(cws[i], dispmsg);
            } else {
                if (sendMsg(to, msg)) {
                    putOutput(cws[i], dispmsg);
                }
            }
            
            free(to);
            free(msg);
            
            return;
        }
    }
}

void flSendAuthKey(Fl_Button *w, void *ignore)
{
    /* sad way to figure out what window we're in ... */
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (cws[i]->bSndKey == w) {
            /* this is our window */
            sendAuthKey(cws[i]->chatWindow->label());
            return;
        }
    }
}

void fSetAway(Fl_Button *w, void *ignore)
{
    awayWindow();
    awayMWin->show();
}

void fAwayMsg(Fl_Input *w, void *ignore)
{
    char *am = strdup(w->value());
    setAway(am);
    bw->bSetAway->color(FL_RED);
    awayMWin->hide();
    free(am);
}

void fBack(Fl_Button *w, void *ignore)
{
    bw->bSetAway->color(FL_GRAY);
    Fl::redraw();
    setAway(NULL);
}

void flDispMsg(const char *window, const char *from, const char *msg, const char *authmsg)
{
    ChatWindow *cw;
    char *dispmsg;
    
    while (!uiLoaded) sleep(0);
    
    wantDisplayLock();
    dn_lock(&displayLock);
    wantLock = false;
    
    dispmsg = (char *) alloca((strlen(from) + (authmsg ? strlen(authmsg) : 0) +
                               strlen(msg) + 7) * sizeof(char));
    if (authmsg) {
        sprintf(dispmsg, "%s [%s]: %s\n", from, authmsg, msg);
    } else {
        sprintf(dispmsg, "%s: %s\n", from, msg);
    }
    
    cw = getWindow(window);
    
    putOutput(cw, dispmsg);
    
    dn_unlock(&displayLock);
}

void flAddRemAutoFind(Fl_Button *btn, void *)
{
    /* sad way to figure out what window we're in ... */
    int i;
    
    for (i = 0; cws[i] != NULL; i++) {
        if (cws[i]->bAutoFind == btn) {
            /* this is our window */
            const char *who = cws[i]->chatWindow->label();
            
            if (checkAutoFind(who)) {
                // this is a removal
                remAutoFind(who);
                btn->label("Add to Autofind List");
                
                // now search through the online list for the user
                for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
                    if (!strcmp(bw->onlineList->text(i) + 2, who)) {
                        // this is the user.  Either unbold or remove an italic entry
                        if (bw->onlineList->text(i)[1] == 'b') {
                            // unbold
                            char *newtext = strdup(bw->onlineList->text(i));
                            if (!newtext) return;
                            newtext[1] = '.';
                            bw->onlineList->text(i, newtext);
                            free(newtext);
                        } else {
                            // remove
                            bw->onlineList->remove(i);
                            olConnsLoc--;
                        }
                        
                        break;
                    }
                }
            } else {
                char found;
                
                // this is an addition
                addAutoFind(who);
                btn->label("Remove from Autofind List");
                
                // now search through the online list for the user
                found = 0;
                for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
                    if (!strcmp(bw->onlineList->text(i) + 2, who)) {
                        char *newtext;
                        
                        found = 1;
                        
                        // this is the user.  Bold it
                        newtext = strdup(bw->onlineList->text(i));
                        if (!newtext) return;
                        newtext[1] = 'b';
                        bw->onlineList->text(i, newtext);
                        free(newtext);
                        
                        break;
                    }
                }
                
                // if the user wasn't in the list, add
                if (!found) {
                    char *utext = (char *) malloc(strlen(who) + 3);
                    if (!utext) return;
                    sprintf(utext, "@i%s", who);
                    bw->onlineList->insert(olConnsLoc, utext);
                    olConnsLoc++;
                    free(utext);
                }
            }
            
            return;
        }
    }
}

void flAddAC(Fl_Input *hostname, void *ignore)
{
    addAutoConnect(hostname->value());
    bw->onlineList->insert(bw->onlineList->size(), hostname->value());
    acw->autoConnWindow->hide();
}

extern "C" void uiDispMsg(const char *from, const char *msg, const char *authmsg, int away)
{
    // for the moment, away messages are undistinguished
    flDispMsg(from, from, msg, authmsg);
}

extern "C" void uiAskAuthImport(const char *from, const char *msg, const char *sig)
{
    int i;
    
    while (!uiLoaded) sleep(0);
    
    char *q = (char *) malloc((strlen(from) + strlen(sig) + 51) * sizeof(char));
    sprintf(q, "%s has asked you to import the key\n'%s'\nDo you accept?", from, sig);
    
    // fix the @ sign, which breaks fl_ask
    for (i = 0; q[i]; i++) {
        if (q[i] == '@') {
            // slide it over
            q = (char *) realloc(q, strlen(q) + 2);
            if (!q) { perror("realloc"); exit(-1); }
            
            memmove(q + i + 2, q + i + 1, strlen(q) - i);
            q[i + 1] = '@';
            i++;
        }
    }
    
    if (flt1_ask(q, 0)) {
        authImport(msg);
    }
    free(q);
}

extern "C" void uiDispChatMsg(const char *chat, const char *from, const char *msg)
{
    char chatWHash[strlen(chat) + 2];
    
    chatWHash[0] = '#';
    strcpy(chatWHash + 1, chat);
    
    flDispMsg(chatWHash, from, msg, NULL);
}

extern "C" void uiEstConn(const char *from)
{
    /* what to use here...? */
}

extern "C" void uiEstRoute(const char *from)
{
    ChatWindow *cw;
    int i, mustadd, addbefore;
    
    while (!uiLoaded) sleep(0);
    
    wantDisplayLock();
    dn_lock(&displayLock);
    wantLock = false;
    
    // Only add if necessary
    mustadd = 1;
    addbefore = olConnsLoc;
    
    for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
        // make sure to add this before any offline users
        if (bw->onlineList->text(i)[1] == 'i' &&
            addbefore > i)
            addbefore = i;
        
        if (!strcmp(bw->onlineList->text(i) + 2, from)) {
            // if this is @i (italic, offline), still must be added
            if (bw->onlineList->text(i)[1] != 'i') {
                mustadd = 0;
            } else {
                bw->onlineList->remove(i);
                olConnsLoc--;
                i--;
            }
        }
    }
    if (mustadd) {
        char *toadd = (char *) malloc(strlen(from) + 3);
        if (!toadd) { perror("malloc"); exit(1); }
        sprintf(toadd, "@%c%s",
                checkAutoFind(from) ? 'b' : '.',
                from);
        bw->onlineList->insert(addbefore, toadd);
        olConnsLoc++;
        free(toadd);
    }
    
    dn_unlock(&displayLock);
}

void removeFromList(const char *name)
{
    int i;
    char demote = 0;
    
    for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
        if (!strcmp(bw->onlineList->text(i) + 2, name)) {
            /* only demote it if it's bolded */
            if (bw->onlineList->text(i)[1] == 'b')
                demote = 1;
            bw->onlineList->remove(i);
            olConnsLoc--;
            break;
        }
    }
    
    if (demote) {
        char *toadd = (char *) malloc(strlen(name) + 3);
        if (!toadd) return;
        sprintf(toadd, "@i%s", name);
        bw->onlineList->insert(olConnsLoc, toadd);
        free(toadd);
    }
}

extern "C" void uiLoseConn(const char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    wantDisplayLock();
    dn_lock(&displayLock);
    wantLock = false;
    
    removeFromList(from);
    
    cw = getWindow(from);
    putOutput(cw, "Connection lost.\n");
    
    dn_unlock(&displayLock);
}

extern "C" void uiLoseRoute(const char *from)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    wantDisplayLock();
    dn_lock(&displayLock);
    wantLock = false;
    
    removeFromList(from);
    
    dn_unlock(&displayLock);
}

extern "C" void uiNoRoute(const char *to)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(to);
    putOutput(cw, "You do not have a route to this user.\n");
}
