/*
 * Copyright 2005, 2006  Gregor Richards
 * Copyright 2006 Bryan Donlan
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "connection.h"
#include "directnet.h"
#include "dnconfig.h"
#include "globals.h"
#include "enc.h"
#include "ui.h"

#include <iostream>
#include <map>
#include <string>
using namespace std;

#include "FL/fl_ask.H"

#include "AutoConnWindow.h"
#include "AutoConnYNWindow.h"
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
map<string, ChatWindow *> cws;
AutoConnWindow *acw;

// keep track of where the buddies end and the connection list begins in onlineLis
int olConnsLoc;

using namespace std;

static int uiQuit = 0;
ChatWindow *showcw = NULL;

int flt1_ask(const char *msg, int t1)
{
    return fl_ask(msg);
}

int main(int argc, char **argv, char **envp)
{
    dn_init(argc, argv);
    
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
    
    /* make the name window */
    nw = new NameWindow();
    nw->make_window();
    nw->nameWindow->show();
    
    Fl::run();
#if 0 
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
#endif

    return 0;
}

ChatWindow *getWindow(const string &name, bool show = true)
{
    int i;
    
    if (cws.find(name) != cws.end()) {
        if (!show && !cws[name]->chatWindow->shown()) return NULL;
        cws[name]->chatWindow->show();
        return cws[name];
    }
    
    if (!show) return NULL;
    
    cws[name] = new ChatWindow();
    cws[name]->make_window();
    cws[name]->chatWindow->label(strdup(name.c_str()));
    
    if (checkAutoFind(name)) {
        cws[name]->bAutoFind->label("Remove from Autofind List");
    } else {
        cws[name]->bAutoFind->label("Add to Autofind List");
    }
    
    cws[name]->chatWindow->show();
    
    return cws[name];
}

void putOutput(ChatWindow *w, const string &txt)
{
    Fl_Text_Buffer *tb = w->textOut->buffer();
    
    w->textOut->insert_position(tb->length());
    w->textOut->insert(txt.c_str());
    w->textOut->show_insert_position();
    Fl::flush();
}

void setName(Fl_Input *w, void *ignore)
{
    int i;
    set<string>::iterator afi, aci;
    
    strncpy(dn_name, w->value(), DN_NAME_LEN);
    dn_name[DN_NAME_LEN] = '\0';
    nw->nameWindow->hide();
    
    /* make the buddy window */
    bw = new BuddyWindow();
    bw->make_window();
    bw->buddyWindow->show();
    
    /* prep the onlineList */
    bw->onlineList->add("@c@mBuddies");
    
    /* add the autofind list */
    for (afi = dn_af_list->begin(); afi != dn_af_list->end(); afi++) {
        string toadd = string("@i") + *afi;
        bw->onlineList->add(toadd.c_str());
    }
    
    /* add the autoconnect list */
    bw->onlineList->add("@c@mAutomatic Connections");
    olConnsLoc = bw->onlineList->size();
    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
        bw->onlineList->add(aci->c_str());
    }
    
    /* finally, the option to add new autoconnections */
    bw->onlineList->add("@c@iAdd new autoconnection");
    
    uiLoaded = 1;

    dn_goOnline();
}

void mainWinClosed(Fl_Double_Window *w, void *ignore)
{
    map<string, ChatWindow *>::iterator cwi;
    
    for (cwi = cws.begin(); cwi != cws.end(); cwi++) {
        cwi->second->chatWindow->hide();
    }
    
    w->hide();
    
    uiQuit = 1;
}

void estConn(Fl_Input *w, void *ignore)
{
    char *connTo;
    AutoConnYNWindow *acynw;
    
    connTo = strdup(w->value());
    w->value("");
    establishConnection(connTo);
    
    if (!checkAutoConnect(connTo)) {
        acynw = new AutoConnYNWindow();
        acynw->hostname = connTo;
        acynw->make_window();
        acynw->autoConnYNWindow->show();
    } else {
        free(connTo);
    }
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
    map<string, ChatWindow *>::iterator cwi;
    
    for (cwi = cws.begin(); cwi != cws.end(); cwi++) {
        if (cwi->second->textIn == w) {
            /* this is our window */
            string dispmsg, to, msg;
            
            to = cwi->second->chatWindow->label();
            
            msg = cwi->second->textIn->value();
            cwi->second->textIn->value("");
            
            dispmsg = dn_name + string(": ") + msg + string("\n");
            
            if (to[0] == '#') {
                // Chat room
                sendChat(to.substr(1), msg);
                putOutput(cwi->second, dispmsg);
            } else {
                if (sendMsg(to, msg)) {
                    putOutput(cwi->second, dispmsg);
                }
            }
            
            return;
        }
    }
}

void flSendAuthKey(Fl_Button *w, void *ignore)
{
    /* sad way to figure out what window we're in ... */
    map<string, ChatWindow *>::iterator cwi;
    
    for (cwi = cws.begin(); cwi != cws.end(); cwi++) {
        if (cwi->second->bSndKey == w) {
            /* this is our window */
            sendAuthKey(cwi->second->chatWindow->label());
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
    string am = w->value();
    setAway(&am);
    bw->bSetAway->color(FL_RED);
    awayMWin->hide();
}

void fBack(Fl_Button *w, void *ignore)
{
    bw->bSetAway->color(FL_GRAY);
    Fl::redraw();
    setAway(NULL);
}

void flDispMsg(const string &window, const string &from, const string &msg, const string &authmsg)
{
    ChatWindow *cw;
    string dispmsg;
    
    assert(uiLoaded);
    
    if (authmsg != "") {
        dispmsg = from + " [" + authmsg + "]: " + msg + "\n";
    } else {
        dispmsg = from + ": " + msg + "\n";
    }
    
    cw = getWindow(window);
    
    putOutput(cw, dispmsg);
}

void flAddRemAutoFind(Fl_Button *btn, void *)
{
    /* sad way to figure out what window we're in ... */
    int i;
    map<string, ChatWindow *>::iterator cwi;
    
    for (cwi = cws.begin(); cwi != cws.end(); cwi++) {
        if (cwi->second->bAutoFind == btn) {
            /* this is our window */
            const char *who = cwi->second->chatWindow->label();
            
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
                    string utext = string("@i") + who;
                    bw->onlineList->insert(olConnsLoc, utext.c_str());
                    olConnsLoc++;
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

void flACYNClose(Fl_Double_Window *w, AutoConnYNWindow *acynw)
{
    acynw->autoConnYNWindow->hide();
    free(acynw->hostname);
    delete acynw;
}

void flACYNYes(Fl_Button *w, AutoConnYNWindow *acynw)
{
    if (!checkAutoConnect(acynw->hostname)) {
        addAutoConnect(acynw->hostname);
        bw->onlineList->insert(bw->onlineList->size(), acynw->hostname);
    }
    acynw->autoConnYNWindow->hide();
    free(acynw->hostname);
    delete acynw;
}

void flACYNNo(Fl_Button *w, AutoConnYNWindow *acynw)
{
    acynw->autoConnYNWindow->hide();
    free(acynw->hostname);
    delete acynw;
}

void uiDispMsg(const string &from, const string &msg, const string &authmsg, int away)
{
    // for the moment, away messages are undistinguished
    flDispMsg(from, from, msg, authmsg);
}

void uiAskAuthImport(const string &from, const string &msg, const string &sig)
{
    int i;
    
    assert(uiLoaded);
    
    string q = from + " has asked you to import the key\n'" + sig + "'\nDo you accept?";
    
    // fix the @ sign, which breaks fl_ask
    for (i = q.find_first_of('@', 0);
         i != string::npos;
         i = q.find_first_of('@', i + 2)) {
        // duplicate it
        q = q.substr(0, i) + "@@" + q.substr(i + 1);
    }
    
    if (fl_ask(q.c_str(), 0)) {
        authImport(msg.c_str());
    }
}

void uiDispChatMsg(const string &chat, const string &from, const string &msg)
{
    flDispMsg(string("#") + chat, from, msg, "");
}

void uiEstConn(const string &from)
{
    /* what to use here...? */
}

void uiEstRoute(const string &from)
{
    int i, mustadd, addbefore;
    ChatWindow *cw;
    
    assert(uiLoaded);
    
    // Only add if necessary
    mustadd = 1;
    addbefore = olConnsLoc;
    
    for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
        // make sure to add this before any offline users
        if (bw->onlineList->text(i)[1] == 'i' &&
            addbefore > i)
            addbefore = i;
        
        if (from == (bw->onlineList->text(i) + 2)) {
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
        string toadd = "@";
        if (checkAutoFind(from)) {
            toadd += "b";
        } else {
            toadd += ".";
        }
        toadd += from;
        bw->onlineList->insert(addbefore, toadd.c_str());
        olConnsLoc++;
    }
    
    cw = getWindow(from, false);
    if (cw) {
        putOutput(cw, "Route established.\n");
    }
}

void removeFromList(const string &name)
{
    int i;
    char demote = 0;
    
    for (i = 2; i < olConnsLoc && bw->onlineList->text(i) != NULL; i++) {
        if (name == (bw->onlineList->text(i) + 2)) {
            /* only demote it if it's bolded */
            if (bw->onlineList->text(i)[1] == 'b')
                demote = 1;
            bw->onlineList->remove(i);
            olConnsLoc--;
            break;
        }
    }
    
    if (demote) {
        string toadd = "@i" + name;
        bw->onlineList->insert(olConnsLoc, toadd.c_str());
        olConnsLoc++;
    }
}

void uiLoseConn(const string &from)
{
    ChatWindow *cw;
    
    assert(uiLoaded);
    
    removeFromList(from);
    
    cw = getWindow(from, false);
    if (cw) {
        putOutput(cw, "Connection lost.\n");
    }
}

void uiLoseRoute(const string &from)
{
    ChatWindow *cw;
    
    assert(uiLoaded);
    
    removeFromList(from);
    
    cw = getWindow(from, false);
    if (cw) {
        putOutput(cw, "Route lost.\n");
    }
}

void uiNoRoute(const string &to)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(to);
    putOutput(cw, "You do not have a route to this user.\n");
}
