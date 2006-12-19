/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
#include "globals.h"
#include "message.h"

map<BinSeq, ChatInfo> dn_chats;

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
 * name: the user */
void chatAddUser(const BinSeq &channel, const BinSeq &name)
{
    if (dn_chats.find(channel) != dh_chats.end()) {
        dn_chats[channel].list.insert(name);
        // FIXME: inform the UI
    }
}

/* Remove a user from my perception of a chat room
 * channel: the channel
 * name: the user */
void chatRemUser(const BinSeq &channel, const BinSeq &name)
{
    if (dn_chats.find(channel) != dh_chats.end()) {
        dn_chats[channel].list.erase(name);
        // FIXME: inform the UI
    }
}

/* Callback for joining a channel, once we've found the owner */
void chatJoinOwnerCallback(const BinSeq &key, const BinSeq &name, void *data)
{
    BinSeq channel = *((BinSeq *) data);
    delete data;
    
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
    Message cjo(1, "cjo", 1, 1);
    cjo.params.push_back(toroute.toBinSeq());
    cjo.params.push_back(rroute.toBinSeq());
    cjo.params.push_back(dn_name);
    cjo.params.push_back(encExportKey());
    cjo.params.push_back(channel);
    
    handleRoutedMsg(cjo);
}

/* Callback for finding info on a channel to join */
void chatJoinDataCallback(const BinSeq &key, const set<BinSeq> &values, void *data)
{
    // there should only be one key
    if (values.size() != 1) {
        // FIXME: inform the UI
        delete data;
        return;
    }
    
    // find the owner
    dhtFindKey(values[0], chatJoinOwnerCallback, data);
}

/* Join a chat
 * channel: the channel */
void chatJoin(const BinSeq &channel)
{
    if (channel.size() &&
        channel[0] == '#') {
        // channel is encoded as an owned channel
        BinSeq chankey("\x01oc", 3);
        chankey += channel.substr(1);
        
        dhtSendAddSearch(chankey, pukeyhash, chatJoinDataCallback, new BinSeq(channel));
    }
}
