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

#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <unistd.h>

int uiInit(int argc, char **argv, char **envp)
{
    initscr();
    addstr("Hello World!\n");
    refresh();
    sleep(5);
    endwin();
    return 0;
}

void uiDispMsg(char *from, char *msg)
{
}

void uiEstConn(char *from)
{
}

void uiEstRoute(char *from)
{
}

void uiLoseConn(char *from)
{
}

void uiLoseRoute(char *from)
{
}

void uiNoRoute(char *to)
{
}
