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

#ifndef DN_CHAT_H
#define DN_CHAT_H

#include <map>
#include <string>
#include <vector>
using namespace std;

extern map<string, vector<string> *> *dn_chats;

/* Am I on this channel?
 * channel: the channel to query
 * returns: 1 if on the channel, 0 otherwise */
char chatOnChannel(const string &channel);

/* Add a user to my perception of a chat room
 * channel: the channel
 * name: the user */
void chatAddUser(const string &channel, const string &name);

/* Remove a user from my perception of a chat room
 * channel: the channel
 * name: the user */
void chatRemUser(const string &channel, const string &name);

/* Join a chat
 * channel: the channel */
void chatJoin(const string &channel);

/* Leave a chat
 * channel: the channel */
void chatLeave(const string &channel);

#endif
