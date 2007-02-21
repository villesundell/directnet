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

#include "auth.h"
#include "chat.h"
#include "dht.h"
#include "directnet.h"
#include "enc.h"
#include "globals.h"
#include "message.h"
#include "ui.h"

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
        
        // make the message
        sendCon(ci);
        
        // inform the UI
        uiDispChatMsg(msg.params[4], "DirectNet", msg.params[1] + " has joined the channel.");
        
    } else if (CMD_IS("Clv", 1, 1)) {
        REQ_PARAMS(4);
        
        // make sure we're in this chat
        if (dn_chats.find(msg.params[3]) == dn_chats.end())
            return;
        ChatInfo &ci = dn_chats[msg.params[3]];
        
        // then make sure we're the owner
        if (!ci.owner)
            return;
        
        // now find the user in the list ...
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
        bool removed = false;
        if (ci.list.find(msg.params[1]) != ci.list.end()) {
            removed = true;
            ci.list.erase(msg.params[1]);
        }
        
        // don't send out a new message if we haven't actually removed anyone
        if (!removed)
            return;
        
        // now send out the new Con
        sendCon(ci);
        
        // and tell the UI
        uiDispChatMsg(msg.params[3], "DirectNet", msg.params[1] + " has left the channel.");
        
    } else if (CMD_IS("Cms", 1, 1)) {
        REQ_PARAMS(5);
        
        BinSeq unencmsg;
        
        if (dn_chats.find(msg.params[2]) == dn_chats.end())
            return;
        ChatInfo &ci = dn_chats[msg.params[2]];
        
        // Decrypt it
        unencmsg = encFrom(msg.params[1], dn_name, msg.params[4]);
        
        // display it
        uiDispChatMsg(msg.params[2], msg.params[3], unencmsg);
        
        // If we're the owner, forward this
        if (ci.owner) {
            msg.params[1] = dn_name;
            map<BinSeq, ChatKeyNameAssoc>::iterator li;
            for (li = ci.list.begin(); li != ci.list.end(); li++) {
                if (li->second.name == msg.params[3]) continue;
                if (li->second.name == dn_name) continue;
                if (dn_routes->find(li->second.key) != dn_routes->end()) {
                    msg.params[0] = (*dn_routes)[li->second.key]->toBinSeq();
                    msg.params[4] = encTo(dn_name, li->second.name, unencmsg);
                    handleRoutedMsg(msg);
                }
            }
        }
        
    } else if (CMD_IS("Con", 1, 1)) {
        REQ_PARAMS(5);
        
        // FIXME: this presumes we actually /wanted/ to be on this channel
        
        ChatInfo &ci = dn_chats[msg.params[3]];
        ci.owner = false;
        ci.name = msg.params[3];
        ci.repkey = msg.params[2];
        ci.repname = msg.params[1];
        
        // now turn the list into join/leave events
        Route rulist(msg.params[4]);
        Route::iterator ri;
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
        
        // 1) Joins
        for (ri = rulist.begin(); ri != rulist.end(); ri++) {
            // if this user is not in the list, we gained one
            bool inlist = (ci.list.find(*ri) != ci.list.end());
            
            if (!inlist) {
                // tell the UI
                // FIXME: better UI message would be nice
                uiDispChatMsg(msg.params[3], "DirectNet", (*ri) + " has joined the channel.");
                
                // gain it
                ci.list[*ri] = ChatKeyNameAssoc("", *ri);
            }
        }
        
        // 2) Parts
findparts:
        for (li = ci.list.begin(); li != ci.list.end(); li++) {
            // if this user is not in the list, we lost one
            bool inlist = rulist.find(li->second.name);
            
            if (!inlist) {
                // tell the UI
                uiDispChatMsg(msg.params[3], "DirectNet", li->second.name + " has left the channel.");
                
                // lose it
                ci.list.erase(li);
                if (ci.list.size() == 0) break;
                goto findparts;
            }
        }
    }
}

/* Send a Con message (for internal use)
 * ci: The channel */
void sendCon(ChatInfo &ci)
{
    // make the message
    Message rmsg(1, "Con", 1, 1);
    rmsg.params.push_back("");
    rmsg.params.push_back(dn_name);
    rmsg.params.push_back(encExportKey());
    rmsg.params.push_back(ci.name);
        
    // make the user list
    Route ulist;
    map<BinSeq, ChatKeyNameAssoc>::iterator li;
    for (li = ci.list.begin(); li != ci.list.end(); li++) {
        ulist.push_back(li->first);
    }
    rmsg.params.push_back(ulist.toBinSeq());
        
    // send it to each user
    for (li = ci.list.begin(); li != ci.list.end(); li++) {
        if (li->first == dn_name) continue;
        if (dn_routes->find(li->second.key) != dn_routes->end()) {
            rmsg.params[0] = (*dn_routes)[li->second.key]->toBinSeq();
            handleRoutedMsg(rmsg);
        }
    }
}

/* Send a chat message (for use by the UI)
 * to: the channel
 * msg: the message */
void sendChat(const BinSeq &to, const BinSeq &msg)
{
    if (dn_chats.find(to) == dn_chats.end()) {
        // we're not even on this chat!
        return;
    }
    
    ChatInfo &ci = dn_chats[to];
    
    // now either send a message to the owner, or to every user
    Message cms(1, "Cms", 1, 1);
    cms.params.push_back("");
    cms.params.push_back(dn_name);
    cms.params.push_back(to);
    cms.params.push_back(dn_name);
    cms.params.push_back("");
    
    if (ci.owner) {
        // we're the owner, so go over the entire list
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
        for (li = ci.list.begin(); li != ci.list.end(); li++) {
            if (li->first == dn_name) continue;
            if (dn_routes->find(li->second.key) != dn_routes->end()) {
                cms.params[0] = (*dn_routes)[li->second.key]->toBinSeq();
                cms.params[4] = encTo(dn_name, li->first, msg);
                handleRoutedMsg(cms);
            }
        }
        
    } else {
        // we're not the owner, so just send it to the owner
        if (dn_routes->find(ci.repkey) != dn_routes->end()) {
            cms.params[0] = (*dn_routes)[ci.repkey]->toBinSeq();
            cms.params[4] = encTo(dn_name, ci.repname, msg);
            handleRoutedMsg(cms);
        }
    }
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
        dn_chats[channel].list[name] = assoc;
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
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
        for (li = ci.list.begin(); li != ci.list.end(); li++) {
            if (li->second.key == key) {
                ci.list.erase(li);
                break;
            }
        }
        // FIXME: inform the UI
    }
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
        set<BinSeq>::iterator vi;
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
        ci.repkey = "";
        ci.repname = "";
        ChatKeyNameAssoc assoc(encExportKey(), dn_name);
        ci.list[dn_name] = assoc;
        
        // FIXME: tell the UI in a better way
        uiDispChatMsg(*rname, "DirectNet", "Channel created.");
        
        // start the refresher (to make sure I keep it)
        
        // send another add, to make sure we keep the channel
        dhtSendAdd(key, pukeyhash, NULL);
        
        delete rname;
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


/* Leave a chat (for use by the UI)
 * channel: the channel */
void chatLeave(const BinSeq &channel)
{
    if (dn_chats.find(channel) == dn_chats.end())
        return; // can't leave a channel we're not in
    
    ChatInfo &ci = dn_chats[channel];
    
    if (ci.owner) {
        // Looks like the chat is dissolving - tell everyone
        
        // make the Con message
        Message con(1, "Con", 1, 1);
        con.params.push_back("");
        con.params.push_back(dn_name);
        con.params.push_back(encExportKey());
        con.params.push_back(channel);
        con.params.push_back(Route().toBinSeq());
        
        // FIXME: this should remove the data
        
        // now send the Con to all users
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
        for (li = ci.list.begin(); li != ci.list.end(); li++) {
            if (li->first == dn_name) continue;
            if (dn_routes->find(li->second.key) != dn_routes->end()) {
                con.params[0] = (*dn_routes)[li->second.key]->toBinSeq();
                handleRoutedMsg(con);
            }
        }
        
    } else {
        // Send a Clv to the owner
        if (dn_routes->find(ci.repkey) != dn_routes->end()) {
            Message clv(1, "Clv", 1, 1);
            clv.params.push_back((*dn_routes)[ci.repkey]->toBinSeq());
            clv.params.push_back(dn_name);
            clv.params.push_back(encExportKey());
            clv.params.push_back(channel);
            handleRoutedMsg(clv);
        }
    }
    
    // either way, remove this channel from our list
    dn_chats.erase(channel);
}

/* When a node is disconnected, call this to remove it from channels
 * key: The node's key */
void chatDis(const BinSeq &key)
{
    // For every chat ...
    map<BinSeq, ChatInfo>::iterator cli;
    for (cli = dn_chats.begin(); cli != dn_chats.end(); cli++) {
        ChatInfo &ci = cli->second;
        if (!ci.owner) continue;
        
        // For every user ...
        bool changed = false;
        map<BinSeq, ChatKeyNameAssoc>::iterator li;
finddisconnects:
        for (li = ci.list.begin(); ci.list.size() && li != ci.list.end(); li++) {
            if (li->second.key == key) {
                // If this user has disconnected, remove it
                
                // Inform the UI (FIXME)
                uiDispChatMsg(ci.name, "DirectNet", li->first + " has left the channel.");
                
                // Then delete the user
                changed = true;
                ci.list.erase(li);
                goto finddisconnects;
            }
        }
        
        if (changed) {
            // some user disconnected, so send the new list
            sendCon(ci);
        }
    }
}
