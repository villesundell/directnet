/*
 * Copyright 2004, 2005, 2006, 2007  Gregor Richards
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

#ifndef DN_CHAT_H
#define DN_CHAT_H

#include <map>
#include <vector>
using namespace std;

#include "binseq.h"

class ChatInfo {
    public:
    bool owner;
    BinSeq name;
    BinSeq rep; // should be a set for unowned?
    set<BinSeq> list; // unimplemented
};

extern map<BinSeq, ChatInfo> dn_chats;

/* Am I on this channel?
 * channel: the channel to query
 * returns: 1 if on the channel, 0 otherwise */
bool chatOnChannel(const BinSeq &channel);

/* Add a user to my perception of a chat room
 * channel: the channel
 * name: the user */
void chatAddUser(const BinSeq &channel, const BinSeq &name);

/* Remove a user from my perception of a chat room
 * channel: the channel
 * name: the user */
void chatRemUser(const BinSeq &channel, const BinSeq &name);

/* Callback for joining a channel, when we've finally successfully joined
 * chan: The channel
 * rep: The owner or representative */
void chatJoined(const BinSeq &chan, const BinSeq &rep)

/* Join a chat
 * channel: the channel */
void chatJoin(const BinSeq &channel);

/* Leave a chat
 * channel: the channel */
void chatLeave(const BinSeq &channel);

#endif
