/*
 * Copyright 2004 Robert Nesius
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

/* Hello World in C for Curses */
/* From http://www.roesler-ac.de/wolfram/hello.htm */
/* The copyright on this code is uncertain, but it's only temporary, a placeholder */

extern "C" {
#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <unistd.h>
#include <lock.h> 
#include <directnet.h>
#include <connection.h>
#include "ui.h"

}

#include <string>
#include <vector>
#include <stdio.h>

using std::string; 
using std::vector;
// #include "Message.h"
#include "DisplayWindow.h" 
#include "StatusWindow.h"
#include "InputWindow.h"
#include "InputParser.h"


DisplayWindow *displayWin; 
StatusWindow *statusWin; 
InputWindow *inputWin; 

// Later we'll have a vector of these with multiple panes for each active chat
// Until then, we are just ui-dumb in curses, so do it this way
string currentPartner = "";
string ldn_name = ""; 

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    // Enter curses mode
    initscr();
    // noecho()  // For when we do our own character input handling
    cbreak(); 
    
    // initialize the screen 
    int max_y, max_x; // in curses, y is always first, so get used to it. 
    getmaxyx(stdscr, max_y, max_x);
    displayWin = new DisplayWindow(max_y -4, max_x);
    statusWin = new StatusWindow(max_x, max_y-4); 
    inputWin = new InputWindow(3, max_x, max_y-3); 
    uiLoaded = 1; // Set the UI flag as ready to go.
    
    string sbuf = "Enter your nickname.  (No spaces)"; 
    dn_lock ( &displayLock); 
    displayWin->displayMsg(sbuf);
    dn_unlock (&displayLock); 
    
    bool nameok = false;
    while (!nameok) { 
        sbuf = inputWin->getInput();
        if (sbuf.length() > 0) { 
            if (sbuf.find(" ",0) == string::npos) { 
                snprintf(dn_name, DN_NAME_LEN-1, "%s", sbuf.c_str());
                dn_name[DN_NAME_LEN - 1] = NULL; 
                sbuf = dn_name;  // In case we shortened it
                ldn_name=sbuf;   // save off the name to a c++ string locally.
                nameok = true; 
            } else { 
                sbuf = "Which part of \"no spaces\" did you not get?"; 
                dn_lock ( &displayLock); 
                displayWin->displayMsg(sbuf);
                dn_unlock (&displayLock);  
            }
        }            
    }
            
    
    dn_name_set = (char) 1; 
    statusWin->setNick(sbuf); 
    sbuf = "/c <host> or <ip> to connect to another machine.\n'/t <username>' to initiate a chat."; 
    dn_lock (&displayLock); 
    displayWin->displayMsg(sbuf); 
    dn_unlock (&displayLock); 
    
    // We're ready to grab input and process it.  
    InputParser inparse;
    bool quitting=false;
    while (!quitting) { 
        sbuf = inputWin->getInput(); 
        if (sbuf.length() > 0) {
            inparse.setInput(sbuf); 
            
            if ( inparse.isCommand() ) {
#ifdef DEBUG
                string tmp = "saw a command"; 
                displayWin->displayMsg(tmp); 
#endif
                switch ( inparse.getCommand() ) { 
                    case CONNECT:
                        establishConnection(inparse.getParam(1).c_str());
                        break;
                    case TALK: 
                        currentPartner = inparse.getParam(1); 
                        statusWin->setTarget(inparse.getParam(1)); 
                        break;
                    case FORCEROUTE:
                        break;
                    case CHAT:
                        break;
                    case QUIT:
                        quitting = true;
                        break;
                    case AWAY: 
                        break;
                        
                }
            } else { // not a command - check for chats later
                if (currentPartner.length() == 0) { 
                    sbuf = "\nYou haven't chosen a chat partner!\nType '/t <username>' to initiate a chat.\n";
                } else { 
                    if ( sendMsg(currentPartner.c_str(), inparse.getInput().c_str()) ) { 
                        uiDispMsg(ldn_name.c_str(), inparse.getInput().c_str() ); 
                    }
                }
                        
            }
        }
        
    }
    
    
    endwin(); 
    delete displayWin;
    delete statusWin;
    delete inputWin;
    return 0;
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    while (!uiLoaded) sleep(0); // Better way to do this?  Better place?
    dn_lock ( &displayLock); 
    string f = from; 
    string m = msg; 
    displayWin->displayRecvdMsg(m, f); 
    statusWin->repaint();
    inputWin->repaint();
    dn_unlock (&displayLock); 
}

extern "C" void uiDispChatMsg(char *chat, char *from, char *msg)
{
}

extern "C" void uiEstConn(char *from)
{
    while (!uiLoaded) sleep(0);
    dn_lock ( &displayLock);
    string f = from;
    string msg = "System: Connection to " + f + "established.";
    displayWin->displayMsg(msg); 
    dn_unlock (&displayLock);
}

extern "C" void uiEstRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    dn_lock ( &displayLock); 
    string f = from; 
    string msg = "System: Route established to " + f; 
    displayWin->displayMsg(msg); 
    dn_unlock (&displayLock);
}

extern "C" void uiLoseConn(char *from)
{
    while (!uiLoaded) sleep(0);
    dn_lock ( &displayLock); 
    string f = from; 
    string msg = "System: Lost connection to " + f; 
    displayWin->displayMsg(msg); 
    dn_unlock (&displayLock);
}

extern "C" void uiLoseRoute(char *from)
{
    while (!uiLoaded) sleep(0);
    dn_lock ( &displayLock); 
    string f = from; 
    string msg = "System: Lost route to " + f; 
    displayWin->displayMsg(msg); 
    dn_unlock (&displayLock);
}

extern "C" void uiNoRoute(char *to)
{
    while (!uiLoaded) sleep(0);
    dn_lock ( &displayLock); 
    string t = to; 
    string msg = "System: No route to " + t; 
    displayWin->displayMsg(msg); 
    dn_unlock (&displayLock);
}
