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

#include "InputWindow.h" 


InputWindow::InputWindow(int nLines, int nCols, int orig_y) 
{
    win = newwin(nLines, nCols, orig_y, 0); 
    scrollok(win, TRUE);
    keypad(win, TRUE);
}

InputWindow::~InputWindow()
{
    delwin(win);
}

string InputWindow::getInput() 
{ 
    char buf[8192]; 
    wclear(win);  
    wmove(win, 0,0);
    wrefresh(win);
    wgetnstr(win, buf, 8192); 
    wclear(win); 
    wmove(win, 0,0);
    wrefresh(win); 
    
    string input(buf); 
    return input;
}