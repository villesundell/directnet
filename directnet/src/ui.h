/*
 * Copyright 2004, 2005  Gregor Richards
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

/* Run the UI
 * argc: argc from main
 * argv: argv from main
 * envp: envp from main
 * returns: 0 on successful run, -1 otherwise */
int uiInit(int argc, char **argv, char **envp);

/* Display a message
 * from: the sender of the message
 * msg: the message
 * authmsg: the authentication message */
void uiDispMsg(char *from, char *msg, char *authmsg);

/* Display a chat message
 * chat: the chat that the message is on
 * from: the person who sent the message
 * msg: the message */
void uiDispChatMsg(char *chat, char *from, char *msg);

/* Display that a connection has been established
 * from: the IP or hostname of the connector */
void uiEstConn(char *from);

/* Display that a route has been established
 * from: the user to whom a route has been established */
void uiEstRoute(char *from);

/* Display that a connection has been lost
 * from: the user (or IP or hostname if no username is available) that has
 *       disconnected */
void uiLoseConn(char *from);

/* Display that a route has been lost
 * from: the user to whom there is no longer a route */
void uiLoseRoute(char *from);

/* Display that a transmission has failed becausethere is no route
 * to: the user that the transmission was sent to */
void uiNoRoute(char *to);

#endif // DN_UI_H
