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
}

#include <string>
using std::string; 
// #include "Message.h"
#include "DisplayWindow.h" 
#include "StatusWindow.h"
#include "InputWindow.h"

DisplayWindow *displayWin; 
StatusWindow *statusWin; 
InputWindow *inputWin; 


extern "C" int uiInit(int argc, char **argv, char **envp)
{
    // Enter curses mode
    initscr();
    // noecho()  // For when we do our own character input handling
    cbreak(); 
    
    // Initialize the lock for output
    dn_lockInit(&displayLock); 
    
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
    
    sbuf = inputWin->getInput();
    // trim leading and trailing whitespace here
    statusWin->setNick(sbuf); 
    sbuf = "\nYou haven't chosen a chat partner!  \nType '/t <username>' to initiate a chat."; 
    dn_lock (&displayLock); 
    displayWin->displayMsg(sbuf); 
    dn_unlock (&displayLock); 
    
    // We're ready to grab input and process it.  
    
    while (1) { 
        sbuf = inputWin->getInput(); 
        
        
    }
        
    
    //
    sleep(5);
    endwin();
    return 0;
}

extern "C" void uiDispMsg(char *from, char *msg)
{
    while (!uiLoaded) sleep(0); // Better way to do this?  Better place?
    dn_lock ( &displayLock); 
    string f = from; 
    string m = msg; 
    displayWin->displayRecvdMsg(m, f); 
    dn_unlock (&displayLock); 
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
