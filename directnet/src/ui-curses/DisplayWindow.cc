/*
 * Copyright 2005 Robert Nesius
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
#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <unistd.h>
}

#include <string>
using std::string;

#include "DisplayWindow.h" 


DisplayWindow::DisplayWindow(int rows, int cols) 
{ 
    nRows = rows; 
    nCols = cols; 
    // Always anchorn display window to top-left corner. 
    win = newwin (nRows, nCols, 0,0);
    scrollok(win, TRUE); 
}

DisplayWindow::~DisplayWindow() 
{ 
    delwin (win);    
}

void DisplayWindow::displayRecvdMsg(string msg, string from) 
{  
    int y, x;  
    
    getsyx( y,x);  // save cursor position so we can put it back
    scroll(win); 
    wmove (win, nRows-1, 0); 
    wprintw (win, "%s: %s", from.c_str(), msg.c_str()); 
    wnoutrefresh(win); 
    // setsyx( y, x); 
    doupdate(); 
} 

void DisplayWindow::displaySentMsg(string msg, string to) 
{ 
    int y, x;  
    
    getsyx( y,x);  // save cursor position so we can put it back
    scroll(win); 
    wmove (win, nRows-1, 0); 
    wprintw (win, "To %s: %s", to.c_str(), msg.c_str()); 
    wnoutrefresh(win); 
    //setsyx( y, x); 
    doupdate(); 
}

void DisplayWindow::displayMsg(string msg) 
{ 
    int y, x;  
    
    getsyx( y,x);  // save cursor position so we can put it back
    scroll(win); 
    wmove (win, nRows-1, 0); 
    // wprintw (win, "Rows: %d     Cols: %d\n", nRows, nCols); 
    wprintw (win, "%s", msg.c_str()); 
    wnoutrefresh(win); 
    setsyx( y, x);
    doupdate(); 
}