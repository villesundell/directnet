/*
 * Copyright 2004 Gregor Richards
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

#ifndef DN_UI_H
#define DN_UI_H

int uiInit(int argc, char **argv, char **envp);

void uiDispMsg(char *from, char *msg);
void uiDispChatMsg(char *chat, char *from, char *msg);
void uiEstConn(char *from);
void uiEstRoute(char *from);
void uiLoseConn(char *from);
void uiLoseRoute(char *from);
void uiNoRoute(char *to);

#endif // DN_UI_H
