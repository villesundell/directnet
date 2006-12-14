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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#ifndef DN_UI_H
#define DN_UI_H

#include <string>
using namespace std;

/* Display a message
 * from: the sender of the message
 * msg: the message
 * authmsg: the authentication message 
 * away: 1 if the user is away*/
void uiDispMsg(const string &from, const string &msg, const string &authmsg, int away);

/* Ask the user if (s)he wants to import a key.
 * from: the sender of the key
 * msg: the key in message format
 * sig: the name in the key */
void uiAskAuthImport(const string &from, const string &msg, const string &sig);

/* Display a chat message
 * chat: the chat that the message is on
 * from: the person who sent the message
 * msg: the message */
void uiDispChatMsg(const string &chat, const string &from, const string &msg);

/* Display that a connection has been established
 * from: the IP or hostname of the connector */
void uiEstConn(const string &from);

/* Display that a route has been established
 * from: the user to whom a route has been established */
void uiEstRoute(const string &from);

/* Display that a connection has been lost
 * from: the user (or IP or hostname if no username is available) that has
 *       disconnected */
void uiLoseConn(const string &from);

/* Display that a route has been lost
 * from: the user to whom there is no longer a route */
void uiLoseRoute(const string &from);

/* Display that a transmission has failed becausethere is no route
 * to: the user that the transmission was sent to */
void uiNoRoute(const string &to);

#endif // DN_UI_H
