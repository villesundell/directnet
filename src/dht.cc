/*
 * Copyright 2006  Gregor Richards
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

#include <string.h>

#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "enc.h"
#include "message.h"
#include "route.h"

map<BinSeq, DHTInfo> in_dhts;

DHTInfo::DHTInfo()
{
    rep = NULL;
    neighbors[0] = NULL; // 0->4 = l->r
    neighbors[1] = NULL;
    neighbors[2] = NULL;
    neighbors[3] = NULL;
    established = false;
}

Route dhtIn(bool must)
{
    Route toret;
    
    // if we need one, make it
    if (must && in_dhts.size() == 0) dhtCreate();
    
    // iterate through all that we're in
    map<BinSeq, DHTInfo>::iterator di;
    
    for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
        toret.push_back(di->first);
    }
    
    return toret;
}

void dhtJoin(const BinSeq &ident, const BinSeq &rep)
{
    in_dhts.erase(ident);
    in_dhts[ident];
}

void dhtCreate()
{
    BinSeq me = encHashKey(encExportKey());
    
    // make it
    DHTInfo &newDHT = in_dhts[encHashKey(encExportKey())];
    
    // establish it
    newDHT.HTI = me;
    newDHT.rep = new BinSeq(me);
    for (int i = 0; i < 4; i++) {
        newDHT.neighbors[i] = new BinSeq(me);
    }
    newDHT.established = true;
}

bool dhtEstablished(const BinSeq &ident)
{
    if (in_dhts.find(ident) == in_dhts.end()) return false;
    return in_dhts[ident].established;
}

void dhtEstablish(const BinSeq &ident)
{
    if (in_dhts.find(ident) == in_dhts.end()) return;
    
    DHTInfo &di = in_dhts[ident];
    
    if (!di.rep) return; // yukk!
    if (!di.neighbors[0]) {
        // send a neighbor request to the representative
        Message msg(1, "Hfn", 1, 1);
        
        if (dn_routes->find(*di.rep) != dn_routes->end())
            msg.params.push_back((*dn_routes)[*di.rep]->toBinSeq());
        else
            return; // uh oh!
        msg.params.push_back(dn_name);
        msg.params.push_back(di.HTI);
        msg.params.push_back(encExportKey());
        msg.params.push_back(Route().toBinSeq());
        msg.params.push_back(encExportKey());

        BinSeq srch;
        srch.push_back('\x03');
        srch.push_back('\x00');
        msg.params.push_back(srch);
        
        handleRoutedMsg(msg);
        
        // TODO: continue
    }
}

BinSeq dhtNextHop(const BinSeq &ident, const BinSeq &key)
{
    // TODO: implement
}

bool dhtForMe(Message &msg, const BinSeq &ident, const BinSeq &key)
{
    if (in_dhts.find(msg.params[2]) == in_dhts.end()) return false;
    BinSeq nextHop = dhtNextHop(msg.params[2], msg.params[3]);
    
    if (nextHop.size()) {
        if (dn_routes->find(nextHop) != dn_routes->end() &&
            (*dn_routes)[nextHop]) {
            msg.params[0] = (*dn_routes)[nextHop]->toBinSeq();
            handleRoutedMsg(msg);
        }
        return false;
    }
    
    return true;
}

#define REQ_PARAMS(x) if (msg.params.size() < x) return
#define CMD_IS(x, y, z) (msg.cmd == x && msg.ver[0] == y && msg.ver[1] == z)

void handleDHTMessage(conn_t *conn, Message &msg)
{
    if (CMD_IS("Hin", 1, 1) ||
        CMD_IS("Hir", 1, 1)) {
        REQ_PARAMS(1);
        
        // same format as routes
        Route *dhtl = new Route(msg.params[0]);
        
        // join all of their DHTs
        for (int i = 0; i < dhtl->size(); i++) {
            if (in_dhts.find((*dhtl)[i]) == in_dhts.end()) {
                if (conn->enckey) {
                    dhtJoin((*dhtl)[i], *(conn->enckey));
                }
            }
        }
        
        delete dhtl;
        
        // echo if applicable
        if (msg.cmd == "Hin") {
            // and hash table info
            Message hmsg(0, "Hir", 1, 1);
            hmsg.params.push_back(dhtIn(true).toBinSeq());
            sendCmd(conn, hmsg);
        }
        
        
    } else if (CMD_IS("Hfn", 1, 1)) {
        // find
        REQ_PARAMS(7);
        
        /*if (dhtForMe(msg, msg.params[2], msg.params[3])) {
            DHTInfo &indht = in_dhts[msg.params[2]];
            
            if (msg.params[6].size() < 2) return;
            
            // we're the destination, what kind of search is it?
            switch (msg.params[6][0]) {
                case '\x03':
                {
                    BinSeq neighbors[4];
                    
                    /* search for successors and predecessors - we are one or
                     * the other, figure out which * /
                    int neg;
                    BinSeq diff = encSub(msg.params[3], encExportKey(), &neg);
                    
                    if (neg) {
                        /* if it's negative, their key is smaller than ours, so
                         * they are a predecessor * /
                        if (indht.neighbors[1]) neighbors[0] = *(indht.neighbors[1]);
                        neighbors[1] = encExportKey();
                        if (indht.neighbors[2]) neighbors[2] = *(indht.neighbors[2]);
                        if (indht.neighbors[3]) neighbors[3] = *(indht.neighbors[3]);
                    } else {
                        /* if it's positive, their key is larger than ours, so
                         * they are a successor * /
                        if (indht.neighbors[0]) neighbors[0] = *(indht.neighbors[0]);
                        if (indht.neighbors[1]) neighbors[1] = *(indht.neighbors[1]);
                        neighbors[2] = encExportKey();
                        if (indht.neighbors[2]) neighbors[3] = *(indht.neighbors[2]);
                    }
                    */
    }
    
    // after handling the command itself, make sure we're established everywhere
    map<BinSeq, DHTInfo>::iterator dhti;
    for (dhti = in_dhts.begin(); dhti != in_dhts.end(); dhti++) {
        if (!dhti->second.established)
            dhtEstablish(dhti->first);
    }
}
