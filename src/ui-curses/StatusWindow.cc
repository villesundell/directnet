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

#include "StatusWindow.h" 

StatusWindow::StatusWindow(int cols, int row_anchor)
{
    win = newwin( 1, cols, row_anchor, 0); 
    cur_nick=""; 
    cur_target=""; 
    ncols = cols;
    redrawStatus();
}

StatusWindow::~StatusWindow() 
{ 
    delwin(win); 
}

void StatusWindow::setNick(string s) 
{
    cur_nick=s;
    redrawStatus();
}

void StatusWindow::setTarget(string s) 
{
    cur_target=s;
    redrawStatus();
}

string StatusWindow::getNick()
{
    return cur_nick;
}

string StatusWindow::getTarget()
{
    return cur_target;
}

void StatusWindow::redrawStatus()
{
    int y, x;  
    
    getsyx( y,x);  // save cursor position so we can put it back
    wmove (win, 0, 0); 
    
    string buf = "---(" + cur_nick + " to " + cur_target + ")"; 
    for (int i = buf.length(); i <= ncols; i++) { 
        buf += "-"; 
    }
    wprintw (win, "%s", buf.c_str()); 
    wnoutrefresh(win); 
    setsyx( y, x); 
    doupdate(); 
}