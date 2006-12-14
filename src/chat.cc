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

#include <map>
#include <string>
using namespace std;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "chat.h"
#include "globals.h"

map<string, vector<string> *> *dn_chats;

char chatOnChannel(const string &channel)
{
    if (dn_chats->find(channel) != dn_chats->end()) {
        return 1;
    } else {
        return 0;
    }
}

void chatAddUser(const string &channel, const string &name)
{
    if (dn_chats->find(channel) == dn_chats->end()) {
        (*dn_chats)[channel] = new vector<string>;
    }
    
    (*dn_chats)[channel]->push_back(name);
}

void chatRemUser(const string &channel, const string &name)
{
    vector<string> *chan;
    int i, s;
    
    if (dn_chats->find(channel) == dn_chats->end()) return;
    
    chan = (*dn_chats)[channel];
    
    // find and remove the user
    s = chan->size();
    for (i = 0; i < s; i++) {
        if ((*chan)[i] == name) {
            chan->erase(chan->begin() + i);
            break;
        }
    }
}

void chatJoin(const string &channel)
{
    if (dn_chats->find(channel) == dn_chats->end())
    {
        (*dn_chats)[channel] = new vector<string>;
    }
}

void chatLeave(const string &channel)
{
    dn_chats->erase(channel);
}
