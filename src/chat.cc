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

#include <iostream>
#include <map>
#include <string>
using namespace std;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "chat.h"
#include "dht.h"
#include "directnet.h"
#include "enc.h"
#include "globals.h"
#include "message.h"

map<BinSeq, ChatInfo> dn_chats;

/* Handle a chat message
 * conn: the connection
 * msg: the message itself */
void handleChatMessage(conn_t *conn, Message &msg)
{
    if (CMD_IS("Cjo", 1, 1)) {
        REQ_PARAMS(5);
        
        // Am I even on this channel?
        if (dn_chats.find(msg.params[4]) == dn_chats.end())
            return;
        
        ChatInfo &ci = dn_chats[msg.params[4]];
        // If it's an owned chat and I'm not the owner, this request is invalid
        // FIXME: only owned chat supported
        if (!ci.owner)
            return;
        
        // FIXME: no way to reject users
        
        // add them
        chatAddUser(msg.params[4], msg.params[3], msg.params[1]);
        cout << "Added " << msg.params[1].c_str() << endl;
        
        // make the message
        Message rmsg(1, "Con", 1, 1);
        rmsg.params.push_back(msg.params[2]);
        rmsg.params.push_back(dn_name);
        rmsg.params.push_back(msg.params[4]);
        
        // make the user list
        Route ulist;
        set<ChatKeyNameAssoc>::iterator li;
        for (li = ci.list.begin(); li != ci.list.end(); li++) {
            ulist.push_back(li->name);
        }
        rmsg.params.push_back(ulist.toBinSeq());
        
        // send it out
        handleRoutedMsg(rmsg);
        
    }
}

/* Send a chat message (for use by the UI)
 * to: the channel
 * msg: the message */
void sendChat(const BinSeq &to, const BinSeq &msg)
{
    // STUB
}

/* Am I on this channel?
 * channel: the channel to query
 * returns: true if on the channel, false otherwise */
bool chatOnChannel(const BinSeq &channel)
{
    if (dn_chats.find(channel) != dn_chats.end()) return true;
    return false;
}

/* Add a user to my perception of a chat room
 * channel: the channel
 * key: the user's key
 * name: the user */
void chatAddUser(const BinSeq &channel, const BinSeq &key, const BinSeq &name)
{
    if (dn_chats.find(channel) != dn_chats.end()) {
        ChatKeyNameAssoc assoc(key, name);
        dn_chats[channel].list.insert(assoc);
        // FIXME: inform the UI
    }
}

/* Remove a user from my perception of a chat room
 * channel: the channel
 * key: the user's key */
void chatRemUser(const BinSeq &channel, const BinSeq &key)
{
    if (dn_chats.find(channel) != dn_chats.end()) {
        // find the matching one
        ChatInfo &ci = dn_chats[channel];
        set<ChatKeyNameAssoc>::iterator li;
        for (li = ci.list.begin();
             li != ci.list.end();
             li++) {
            if (li->key == key) {
                ci.list.erase(li);
                break;
            }
        }
        // FIXME: inform the UI
    }
}

/* Callback for joining a channel, when we've finally successfully joined
 * chan: The channel
 * rep: The owner or representative */
void chatJoined(const BinSeq &chan, const BinSeq &rep)
{
    if (dn_chats.find(chan) != dn_chats.end())
        dn_chats.erase(chan);
    
    ChatInfo &ci = dn_chats[chan];
    ci.owner = false;
    ci.name = chan;
    ci.rep = rep;
    
    // FIXME: tell the UI
}

/* Callback for joining a channel, once we've found the owner */
void chatJoinOwnerCallback(const BinSeq &key, const BinSeq &name, void *data)
{
    BinSeq channel = *((BinSeq *) data);
    delete (BinSeq *) data;
    
    if (name == "") {
        // FIXME: inform the UI
        return;
    }
    
    Route toroute, rroute;
    
    if (dn_routes->find(key) != dn_routes->end()) {
        toroute = *((*dn_routes)[key]);
    }
    
    rroute = toroute;
    if (rroute.size()) rroute.pop_back();
    rroute.reverse();
    rroute.push_back(pukeyhash);
    
    // send a join request
    Message cjo(1, "Cjo", 1, 1);
    cjo.params.push_back(toroute.toBinSeq());
    cjo.params.push_back(dn_name);
    cjo.params.push_back(rroute.toBinSeq());
    cjo.params.push_back(encExportKey());
    cjo.params.push_back(channel);
    
    handleRoutedMsg(cjo);
}

/* Callback for finding info on a channel to join */
void chatJoinDataCallback(const BinSeq &key, const set<BinSeq> &values, void *data)
{
    // there should only be one value
    if (values.size() != 1) {
        // FIXME: inform the UI
        delete (BinSeq *) data;
        return;
    }
    
    // get the room name
    BinSeq *rname = (BinSeq *) data;
    
    // if I'm the owner, just create the chat
    BinSeq value = *(values.begin());
    if (value == pukeyhash) {
        if (dn_chats.find(*rname) != dn_chats.end())
            dn_chats.erase(*rname);
        
        ChatInfo &ci = dn_chats[*rname];
        ci.owner = true;
        ci.name = *rname;
        ci.rep = pukeyhash;
        
        delete rname;
        
        // FIXME: tell the UI
    } else {
        // find the owner
        dhtFindKey(value, chatJoinOwnerCallback, data);
    }
}

/* Join a chat (for use by the UI)
 * channel: the channel */
void chatJoin(const BinSeq &channel)
{
    if (channel.size() &&
        channel[0] == '#') {
        // channel is encoded as an owned channel
        BinSeq chankey("\x01oc", 3);
        chankey += channel.substr(1);
        
        dhtSendAddSearch(chankey, pukeyhash, chatJoinDataCallback, new BinSeq(channel));
        
    } else {
        // FIXME: inform the UI
    }
}
