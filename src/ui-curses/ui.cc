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
}

// #include "Message.h"
#include "DisplayWindow.h" 
#include "StatusWindow.h"
#include "InputWindow.h"

extern "C" int uiInit(int argc, char **argv, char **envp)
{
    // Enter curses mode
    initscr();
    // noecho()  // For when we do our own character input handling
    cbreak(); 
   
    // Get the screen dimensions.  Probably this should all move into a "screen" object.
    int max_y, max_x; // in curses, y is always first, so get used to it. 
    getmaxyx(stdscr, max_y, max_x);
    DisplayWindow displayWin(max_y -5, max_x);
    StatusWindow statusWin(max_x, max_y-4); 
    InputWindow inputWin(3, max_x, max_y-3); 
    
    string sbuf = "Enter your nickname"; 
    displayWin.displayMsg(sbuf); 
    sbuf = inputWin.getInput();
    statusWin.setNick(sbuf); 
    sbuf = "You haven't chosen a chat partner!  Type '/t <username>' to initiate a chat."; 
    displayWin.displayMsg(sbuf); 
    
    //
    sleep(5);
    endwin();
    return 0;
}

extern "C" void uiDispMsg(char *from, char *msg)
{
}

extern "C" void uiEstConn(char *from)
{
}

extern "C" void uiEstRoute(char *from)
{
}

extern "C" void uiLoseConn(char *from)
{
}

extern "C" void uiLoseRoute(char *from)
{
}

extern "C" void uiNoRoute(char *to)
{
}
