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
#include <set>
using namespace std;

#include "binseq.h"
#include "connection.h"

class ChatKeyNameAssoc {
    public:
    inline ChatKeyNameAssoc(const BinSeq &skey, const BinSeq &sname) {
        key = skey;
        name = sname;
    }
    inline bool operator<(const ChatKeyNameAssoc &to) const
    { return (key != "" ? key < to.key : name < to.name); }
    inline bool operator>(const ChatKeyNameAssoc &to) const
    { return (key != "" ? key > to.key : name < to.name); }
    inline bool operator==(const ChatKeyNameAssoc &to) const
    { return (key != "" ? key == to.key : name < to.name); }
    BinSeq key;
    BinSeq name;
};

class ChatInfo {
    public:
    bool owner;
    BinSeq name;
    BinSeq repkey; // should be a set for unowned?
    BinSeq repname;
    set<ChatKeyNameAssoc> list; // list of keys, names
};

extern map<BinSeq, ChatInfo> dn_chats;

/* Handle a chat message
 * conn: the connection
 * msg: the message itself */
void handleChatMessage(conn_t *conn, Message &msg);

/* Send a Con message (for internal use)
 * ci: The channel */
void sendCon(ChatInfo &ci);

/* Send a chat message (for use by the UI)
 * to: the channel
 * msg: the message */
void sendChat(const BinSeq &to, const BinSeq &msg);

/* Am I on this channel?
 * channel: the channel to query
 * returns: 1 if on the channel, 0 otherwise */
bool chatOnChannel(const BinSeq &channel);

/* Add a user to my perception of a chat room
 * channel: the channel
 * key: the user's key
 * name: the user */
void chatAddUser(const BinSeq &channel, const BinSeq &key, const BinSeq &name);

/* Remove a user from my perception of a chat room
 * channel: the channel
 * key: the user's key */
void chatRemUser(const BinSeq &channel, const BinSeq &key);

/* Join a chat (for use by the UI)
 * channel: the channel */
void chatJoin(const BinSeq &channel);

/* Leave a chat (for use by the UI)
 * channel: the channel */
void chatLeave(const BinSeq &channel);

/* When a node is disconnected, call this to remove it from channels
 * key: The node's key */
void chatDis(const BinSeq &key);

#endif
