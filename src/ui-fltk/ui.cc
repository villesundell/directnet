/*
 * Copyright 2005, 2006, 2007  Gregor Richards
 * Copyright 2006  Bryan Donlan
 * Copyright 2006  "Solarius"
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
using namespace std;

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>//We need this for timestamp(s)

#include "auth.h"
#include "chat.h"
#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dnconfig.h"
#include "globals.h"
#include "enc.h"
#include "ui.h"

#include <iostream>
#include <map>
#include <string>
#include <sstream>
using namespace std;

#include "FL/fl_ask.H"
#include <FL/Fl_Menu.H>
#include <FL/Fl_Window.H>

#include "AutoConnWindow.h"
#include "AutoConnYNWindow.h"
#include "BuddyWindow.h"
#include "ChatWindow.h"
#include "DHTWindow.h"
#include "NameWindow.h"
#include "QWindow.h"

#ifdef __WIN32
#define sleep _sleep
#include <malloc.h>
#endif

NameWindow *nw;
BuddyWindow *bw;
map<string, ChatWindow *> cws;
AutoConnWindow *acw;
Fl_Window *miniwin, *notewin;
string msgbuffer="";

//Do we want minimode?
int enable_minimode = 0; //Type: 'directnet -m' to enable it
Fl_Button *minib = NULL; //That big button;)
void minimodeCallback (Fl_Button*);//Minimodes button callback
int mini_msg_count=0;
void updateMinimode ();

//For Timestamps:
time_t rawtime;
tm *timestamp;

// Decls
void showConnWindow(Fl_Widget *w, void *ignore);
void estConn(Fl_Widget *w, void *data);
void showFindWindow(Fl_Widget *w, void *ignore);
void estFnd(Fl_Widget *w, void *ignore);
void showChatWindow(Fl_Widget *w, void *ignore);
void estChat(Fl_Widget *w, void *ignore);
void showAwayWindow(Fl_Widget *w, void *ignore);
void flSetAway(Fl_Widget *w, void *data);
void setBack(Fl_Widget *w, void *data);
void dhtAction(string label);
void dhtTimeUpdate(dn_event_timer *dte);

//Banning:
void flBanUnban (Fl_Button*, void*);

// keep track of where the buddies end and the connection list begins in onlineLis
int olConnsLoc;

//For html widget
void putError (ChatWindow *w, const string &txt);

using namespace std;

static int uiQuit = 0;
ChatWindow *showcw = NULL;

int flt1_ask(const char *msg, int t1)
{
    return fl_ask(msg);
}

//Here we make timestamps
char* make_timestamp ()
{
    //here happens timestamping:
    static char timestr[10];
    time (&rawtime); //rawtime is declared in the start of this file
    //timestamp = localtime (&rawtime); //Timestamp is declared there too;)
    strftime (timestr, sizeof (timestr), "%H:%M ", localtime (&rawtime));
    return timestr;
}

int main(int argc, char **argv, char **envp)
{
    char *cmd_nm = NULL;
    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            if (!strncmp(argv[1], "-m", 2)) {
                enable_minimode = 1;
            } else if (!strncmp(argv[i], "-n", 2)) {
                i++;
                cmd_nm = argv[i];
            }
        }
    }
    dn_init(argc, argv);
    

    //Here we show release notes
    #ifdef DN_RELEASENOTES
    notewin = new Fl_Window (300, 400, "Release notes");
    Fl_Group::current()->resizable(notewin);
    Fl_Help_View *noteview = new Fl_Help_View(0, 0, 300, 400, "hello");
    noteview->value(DN_RELEASENOTES);
    notewin->end();
    notewin->show();
    #endif
    
    /* Always start by finding encryption */
    if (findEnc(envp) == -1) {
        printf("No encryption binaries were found on your PATH!\n");
        return -1;
    }
    
#if 0 // authentication doesn't work right now
#ifndef __WIN32
    /* Then authentication */
    if (!authInit()) {
        printf("Authentication failed to initialize!\n");
        return -1;
    } else if (authNeedPW() && !cmd_nm) {
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
    } else if (!cmd_nm) {
        authSetPW("", "");
    }
#else
    authSetPW("", "");
#endif
#endif
    authSetPW("", "");
    
    /* And creating the key */
    encCreateKey();
    
    /* make the name window */
    nw = new NameWindow();
    nw->make_window();
    nw->nameWindow->show();
    if (cmd_nm) {
        // set the name 
        nw->nameIn->value(cmd_nm);
        setName(nw->nameIn, NULL);
    }
    
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
    
    //Check, do 
    //if (minib)
    //{
	if (cws.find(name) != cws.end()) {
		if (!show && !cws[name]->chatWindow->shown()) return NULL;
		cws[name]->chatWindow->show();
		return cws[name];
	}
    //}
    
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
    //Fl_Text_Buffer *tb = w->textOut->buffer();
    string strippedstr; //Place for stripped text
    char *msgtimestamp = ""; //Here is messages timestamp
    
    if (w->textOut->value() == NULL)
    {
    	w->textOut->value(" ");
    }
    //Now we should stripout all nonpermitted html-tags
    //Here we rip all bad html tags off
    string colorstr="";
    int strlen = txt.length();
    if (strlen>4)
    {
    	//If string is over 4 character long, we continue
    	//I tried to minimize risk of buffer mallfunctions in every stage, so thats why i am checking lenghts every time;)
    	int i=0;
    	int i2;
	for (i=0;i<strlen;i++)
	{
            if (txt[i]=='<')
            {
                //We found html tag
                if ((strlen-i)>=2)
                {
                    if (txt.substr(i,3)=="<b>"||txt.substr(i,3)=="</b"||txt.substr(i,3)=="<i>"||txt.substr(i,3)=="</i"||txt.substr(i,3)=="<u>"||txt.substr(i,3)=="</u"||txt.substr(i,4)=="<br>")
                    {
                        //This tag is legal, so we dont strip that
                        strippedstr+=txt[i];
                    }
                    else if (txt.substr(i,2)=="<#")
                    {
                        //This is Gregs colortag-idea (<#blue> is replaced as <font color="blue"> and <#> is replaced as </font> (Solarius' addition)
                        colorstr = "";
                        for (i2=2;i2<10;i2++)
                        {
                            if ((i2+i)<strlen)
                            {
                                //we have space to read
                                if (txt[i+i2]=='>')
                                {
                                    //We should leave this loop now
                                    break;
                                }
                                else
                                {
                                    colorstr+=txt[i+i2];
                                }
                            }
                            else
                            {
                                //We should leave this loop
                                break;
                            }
                        }
                        //Move "carret":
                        i = i+i2;
                        if (colorstr=="")
                        {
                            //Colorstring is empty
                            strippedstr+="</font>";
                        }
                        else
                        {
                            //we have color
                            strippedstr+="<font color="+colorstr+">";
                        }
                    }
                    else
                    {
                        strippedstr+="&lt;";
                    }
                }
                else
                {
                    strippedstr+="&lt;";
                }
            }
            else
            {
                strippedstr+=txt[i];
            }
	}

    }
    
    //Well, _very_ lazy (and non-standard) way to secure our html-tags;) (but hey, it works;))
    strippedstr+="</font></b></i></u>";
    
    // get the output
    string htmloutput;
    htmloutput = w->textOut->value();
    htmloutput += "<br><font color=gray>";
    htmloutput += string(make_timestamp()) + "</font>" + strippedstr;
    w->textOut->value(htmloutput.c_str());
    ((Fl_Valuator*) &(w->textOut->scrollbar_))->value(
        w->textOut->scrollbar_.maximum());
    w->textOut->topline(w->textOut->scrollbar_.value());
    
    //Adding one to counter (for minimode):
    mini_msg_count++;
    updateMinimode ();
    
    Fl::flush();
}

void putError (ChatWindow *w, const string &txt)
{
	//Put error to screen
	putOutput(w, "<b><#red>"+txt+"<#></b><br>");
}

// Update the status bar
void updateStatus()
{
    if (!bw) return;
    
    // status string: {Online,Offline}[ - Away]
    string stat;
    
    // 1) Online/offline
    if (dnOnline()) {
        stat = "Online";
    } else {
        stat = "Offline";
    }
    
    // 2) Away
    const string *ga = getAway();
    if (ga) {
        stat += " - Away: " + *ga;
    }
    
    bw->status->value(stat.c_str());
}

// set the name and show the main window
void setName(Fl_Input *w, void *ignore)
{
    int i;
    set<string>::iterator afi, aci;
    
    // we no longer need the release-notes window
    if (notewin)
        notewin->hide();
    
    strncpy(dn_name, w->value(), DN_NAME_LEN);
    dn_name[DN_NAME_LEN] = '\0';
    nw->nameWindow->hide();
    
    /* make the buddy window */
    bw = new BuddyWindow();
    bw->make_window();
    
    /* Make the menu bar */
    Fl_Menu_Item menu[] = {
        { "&Network", 0, 0, 0, FL_SUBMENU },
        { "New &Connection", FL_CTRL + 'n', (Fl_Callback *) showConnWindow },
        { "Set &Away Message", FL_CTRL + 'a', (Fl_Callback *) showAwayWindow },
        { "Unset Away", FL_CTRL + 'b', (Fl_Callback *) setBack },
        { "&Quit", FL_CTRL + 'q', (Fl_Callback *) mainWinClosed },
        { 0 },
    
        { "&Users", 0, 0, 0, FL_SUBMENU },
        { "&Find User", FL_CTRL + 'f', (Fl_Callback *) showFindWindow },
        { 0 },
    
        { "&Chat", 0, 0, 0, FL_SUBMENU },
        { "&Join Chat Channel", FL_CTRL + 'j', (Fl_Callback *) showChatWindow },
        { 0 },
    
        { 0 }
    };
    bw->mBar->copy(menu);
    
    if (enable_minimode == 0)
    {
    	// Standard mode
    	bw->buddyWindow->show();
    }
    else
    {
    	// Mini-mode
    	miniwin = new Fl_Window (100, 100, "DNMini");
    	Fl_Group::current()->resizable(miniwin);
    	minib = new Fl_Button (0, 0, 100, 100, "DN");
    	minib->callback ((Fl_Callback*)minimodeCallback);
    	Fl_Group::current()->resizable(minib);
    	miniwin->end();
    	miniwin->show();
    }
    
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
    
    // get the initial status
    updateStatus();
    
    uiLoaded = 1;

    dn_goOnline();
}

void mainWinClosed(Fl_Double_Window *w, void *ignore)
{
    map<string, ChatWindow *>::iterator cwi;
    
    for (cwi = cws.begin(); cwi != cws.end(); cwi++) {
        cwi->second->chatWindow->hide();
    }
    
    if (bw && bw->buddyWindow) {
        bw->buddyWindow->hide();
    }
    
    uiQuit = 1;
}

// <CONNECTION>
void showConnWindow(Fl_Widget *w, void *ignore)
{
    QWindow *connWin = new QWindow();
    connWin->make_window();
    connWin->qWin->label("DirectNet Connection");
    connWin->qInp->label("Hostname to connect to:");
    connWin->qInp->callback((Fl_Callback *) estConn, (void *) connWin);
    connWin->okButton->callback((Fl_Callback *) estConn, (void *) connWin);
    connWin->qWin->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->cancelButton->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->qWin->show();
}

void estConn(Fl_Widget *w, void *data)
{
    char *connTo;
    AutoConnYNWindow *acynw;
    
    if (!data) return;
    
    // get the input ...
    QWindow *connWin = (QWindow *) data;
    connTo = strdup(connWin->qInp->value());
    
    // get rid of the window
    connWin->qWin->hide();
    delete connWin->qWin;
    delete connWin;
    
    // of course, establish the connection
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
// </CONNECTION>

// <FIND>
void showFindWindow(Fl_Widget *w, void *ignore)
{
    QWindow *connWin = new QWindow();
    connWin->make_window();
    connWin->qWin->label("Find User");
    connWin->qInp->label("User to search for:");
    connWin->qInp->callback((Fl_Callback *) estFnd, (void *) connWin);
    connWin->okButton->callback((Fl_Callback *) estFnd, (void *) connWin);
    connWin->qWin->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->cancelButton->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->qWin->show();
}

void estFnd(Fl_Widget *w, void *data)
{
    char *findU;
    
    if (!data) return;
    
    // get the input ...
    QWindow *connWin = (QWindow *) data;
    findU = strdup(connWin->qInp->value());
    
    // get rid of the window
    connWin->qWin->hide();
    delete connWin->qWin;
    delete connWin;
    
    // of course, establish the connection
    sendFnd(findU);
    dhtAction(string("Searching for user ") + findU);
    free(findU);
}
// </FIND>

// <CHAT>
void showChatWindow(Fl_Widget *w, void *ignore)
{
    QWindow *connWin = new QWindow();
    connWin->make_window();
    connWin->qWin->label("Join Chat");
    connWin->qInp->label("Chat to join:");
    connWin->qInp->callback((Fl_Callback *) estChat, (void *) connWin);
    connWin->okButton->callback((Fl_Callback *) estChat, (void *) connWin);
    connWin->qWin->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->cancelButton->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->qWin->show();
}

void estChat(Fl_Widget *w, void *data)
{
    char *joinC;
    
    if (!data) return;
    
    // get the input ...
    QWindow *connWin = (QWindow *) data;
    joinC = strdup(connWin->qInp->value());
    
    if (joinC[0] != '#')
        return; // invalid
    
    // get rid of the window
    connWin->qWin->hide();
    delete connWin->qWin;
    delete connWin;
    
    // of course, join the chat
    chatJoin(joinC);
    dhtAction(string("Joining ") + joinC);
    ChatWindow *cw = getWindow(joinC);
    
    // now hide the useless buttons
    cw->bBan->hide();
    cw->bSndKey->hide();
    cw->bAutoFind->hide();
    
    free(joinC);
}

void closeChat(Fl_Double_Window *w, void *ignore)
{
    char *name;
    
    name = strdup(w->label());
    
    // if it's a chat window, we should leave the chat now
    if (name[0] == '#') {
        chatLeave(name);
    }
    
    w->hide();
    
    free(name);
}
// </CHAT>

// <AWAY>
void showAwayWindow(Fl_Widget *w, void *ignore)
{
    QWindow *connWin = new QWindow();
    connWin->make_window();
    connWin->qWin->label("Away");
    connWin->qInp->label("Away message:");
    connWin->qInp->callback((Fl_Callback *) flSetAway, (void *) connWin);
    connWin->okButton->callback((Fl_Callback *) flSetAway, (void *) connWin);
    connWin->qWin->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->cancelButton->callback((Fl_Callback *) destroyQWin, (void *) connWin);
    connWin->qWin->show();
}

void flSetAway(Fl_Widget *w, void *data)
{
    string awayMsg;
    
    if (!data) return;
    
    // get the input ...
    QWindow *connWin = (QWindow *) data;
    awayMsg = connWin->qInp->value();
    
    // get rid of the window
    connWin->qWin->hide();
    delete connWin->qWin;
    delete connWin;
    
    // set away
    if (awayMsg != "") {
        setAway(&awayMsg);
    } else {
        setAway(NULL);
    }
    
    updateStatus();
}

void setBack(Fl_Widget *w, void *data)
{
    setAway(NULL);
    updateStatus();
}
// </AWAY>

// display the DHT Action window and perform an action
void dhtAction(string label)
{
    DHTWindow *dw = new DHTWindow();
    dw->make_window();
    
    dw->tWin->copy_label(label.c_str());
    dw->tBar->copy_label(label.c_str());
    dw->tWin->show();
    
    // start the timer
    dn_event_timer *dte = new dn_event_timer(
        0, 100000,
        dhtTimeUpdate, dw);
    dte->activate();
}

void dhtTimeUpdate(dn_event_timer *dte)
{
    dte->setTimeDelta(0, 100000);
    DHTWindow *dw = (DHTWindow *) dte->payload;
    
    // update the bar
    float at = dw->tBar->value();
    at += 1.0;
    if (at >= 100.0) {
        // we're done
        dte->deactivate();
        delete dte;
        
        dw->tWin->hide();
        delete dw->tWin;
        delete dw;
    } else {
        dw->tBar->value(at);
    }
}

void flSelectBuddy(Fl_Browser *flb, void *ignore)
{
    static int previous = 0;
    int i = flb->value();
    
    // ignore selection of the headers
    if (i == 1) {
        flb->deselect();
        
        // get rid of any display in the chat button
        bw->chatButton->label("");
        bw->chatButton->deactivate();
        
        return;
    } else if (i == olConnsLoc) {
        // go either above or below
        if (previous > olConnsLoc) {
            i--;
            previous = i;
            flb->value(i);
            flSelectBuddy(flb, ignore);
            return;
        } else {
            i++;
            flb->value(i);
        }
    }
    
    previous = i;
    
    // change the chat button depending on where in the list we are
    if (i < olConnsLoc) {
        bw->chatButton->label("Chat");
    } else if (i < flb->size()) {
        bw->chatButton->label("Remove from Autoconnection List");
    } else {
        bw->chatButton->label("Add new Autoconnection");
    }
    bw->chatButton->activate();
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
            
            dispmsg = string("<b>")+dn_name+ string("</b>") + string(": ") + msg + string("<br>");
            
            if (to[0] == '#') {
                // Chat room
                sendChat(to, msg);
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

void flDispMsg(const string &window, const string &from, const string &msg, const string &authmsg)
{
    ChatWindow *cw;
    string dispmsg;

    assert(uiLoaded);
    
    if (authmsg != "") {
        dispmsg = "<b>" + from + "</b>" + "<#gray> [" + authmsg + "]<#>: " + msg + "<br>";
    } else {
        dispmsg = "<b>" + from + "</b>" + ": " + msg + "<br>";
    }
    
    cw = getWindow(window);
    if (cw->bBan->color()!=FL_RED)
    {
    	//If we havent banned this user, we should display that message:
    	putOutput(cw, dispmsg);
    }
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
    flDispMsg(chat, from, msg, "");
}

void uiEstConn(const string &from)
{
    // FIXME: this desperately needs to change
    string cemsg = "Connection to " + from + " established.";
    uiDispMsg("DirectNet", cemsg, "", 0);
    updateStatus();
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
        putError(cw, "Route established.\n");
    }
    updateStatus();
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
        putError(cw, "Connection lost.\n");
    }
    updateStatus();
}

void uiLoseRoute(const string &from)
{
    ChatWindow *cw;
    
    assert(uiLoaded);
    
    removeFromList(from);
    
    cw = getWindow(from, false);
    if (cw) {
        putError(cw, "Route lost.\n");
    }
    updateStatus();
}

void uiNoRoute(const string &to)
{
    ChatWindow *cw;
    
    while (!uiLoaded) sleep(0);
    
    cw = getWindow(to);
    putError(cw, "You do not have a route to this user.\n");
}

//Minimode function:
void minimodeCallback (Fl_Button *b)
{
	//Minimode button have been clicked!
	if (bw->buddyWindow->visible())
	{
		bw->buddyWindow->hide();
		minib->label("(DN)");
	}
	else
	{
		bw->buddyWindow->show();
		mini_msg_count=0;
		minib->label("DN");
	}
	updateMinimode ();
}

void updateMinimode ()
{
    if (!minib) return;
    //Here is update code
    //mini_msg_count++;
    //string temps1 = "moi"+string(mini_msg_count);
    //minib->label(temps1.c_str());
    if (mini_msg_count>0)
    {
        //We shall color that button red
        minib->color(FL_RED);
    }
    else
    {
        //It is normal state
        minib->color(FL_GRAY);
    }
    minib->redraw();
}

//Ban/Unban function:
void flBanUnban (Fl_Button *b, void *)
{
	if (b->color()==FL_RED)
	{
		b->color(FL_BACKGROUND_COLOR);
		b->label ("Block");
	}
	else
	{
		b->color(FL_RED);
		b->label ("Unblock");
	}
}
