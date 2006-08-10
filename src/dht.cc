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

#include <iostream>
using namespace std;

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
    nbors_keys[0] = NULL;
    nbors_keys[1] = NULL;
    nbors_keys[2] = NULL;
    nbors_keys[3] = NULL;
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
    DHTInfo &dhti = in_dhts[ident];
    dhti.rep = new BinSeq(rep);
    dhti.HTI = ident;
}

void dhtCreate()
{
    // make it
    DHTInfo &newDHT = in_dhts[pukeyhash];
    
    // establish it
    newDHT.HTI = pukeyhash;
    newDHT.rep = new BinSeq(encExportKey());
    for (int i = 0; i < 4; i++) {
        newDHT.neighbors[i] = new BinSeq(pukeyhash);
        newDHT.nbors_keys[i] = new BinSeq(encExportKey());
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
        BinSeq rt;
        
        if (dn_routes->find(*di.rep) != dn_routes->end())
            rt = (*dn_routes)[*di.rep]->toBinSeq();
        else
            return; // uh oh!
        
        msg.params.push_back(rt);
        msg.params.push_back(dn_name);
        msg.params.push_back(di.HTI);
        msg.params.push_back(pukeyhash);
        msg.params.push_back(rt);
        msg.params.push_back(encExportKey());
        
        BinSeq srch;
        srch.push_back('\x03');
        srch.push_back('\x00');
        msg.params.push_back(srch);
        
        handleRoutedMsg(msg);
        
        // TODO: continue
    }
}

void outHex(const char *dat, int count)
{
    for (int i = 0; i < count; i++) {
        printf("%.2X", ((const unsigned char *) dat)[i]);
    }
}

void dhtDebug(const BinSeq &ident)
{
    if (in_dhts.find(ident) == in_dhts.end()) return;
    
    DHTInfo &di = in_dhts[ident];
    
    cout << "Key: ";
    BinSeq key = encExportKey();
    outHex(key.c_str(), key.size());
    cout << endl;
    
    cout << "Hash: ";
    outHex(pukeyhash.c_str(), pukeyhash.size());
    cout << endl;
    
    cout << "DHT: ";
    outHex(ident.c_str(), ident.size());
    cout << endl;
    
    cout << "  Established: " << (di.established ? "Yes" : "No") << endl;
    
    cout << "  Representative: ";
    if (di.rep) {
        outHex(di.rep->c_str(), di.rep->size());
    } else {
        cout << "None";
    }
    cout << endl;
    
    cout << "  Neighbors:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "   " << i << ": ";
        if (di.neighbors[i]) {
            outHex(di.neighbors[i]->c_str(), di.neighbors[i]->size());
        } else {
            cout << "None";
        }
        cout << endl;
        if (i == 1) {
            cout << "   *: ";
            outHex(pukeyhash.c_str(), pukeyhash.size());
            cout << endl;
        }
    }
    
    cout << "--------------------------------------------------" << endl << endl;
}

BinSeq dhtNextHop(const BinSeq &ident, const BinSeq &key, bool noexact)
{
    if (in_dhts.find(ident) == in_dhts.end()) return "";
    DHTInfo &dhi = in_dhts[ident];
    
    // if noexact is set, we will only accept less than, not less than or equal to
    int accpt = 0;
    if (noexact) accpt = -1;
    
    if (!dhi.established) {
        if (!dhi.neighbors[2]) {
            // just send it to the rep
            if (!dhi.rep) return "";
            return *(dhi.rep);
        }
        
        for (int i = 2; i >= 1; i--) {
            if (dhi.neighbors[i]) {
                // check if we need it more than they do
                if (encCmp(*(dhi.neighbors[i]), key) <= accpt) {
                    // it's for them
                    if (*(dhi.neighbors[i]) != pukeyhash) {
                        return *(dhi.nbors_keys[i]);
                    } else {
                        return "";
                    }
                }
            }
        }
        
        return "";
    } else {
        
        for (int i = 0; i < dhi.real_divisions.size(); i++) {
            if (dhi.real_divisions[i]) {
                if (encCmp(*(dhi.real_divisions[i]), key) <= accpt) {
                    // this is the next hop
                    if (*(dhi.real_divisions[i]) != pukeyhash) {
                        return *(dhi.real_divs_keys[i]);
                    } else {
                        return "";
                    }
                }
            }
        }
        
        // check neighbors in case the divisions aren't up yet
        for (int i = 2; i >= 1; i--) {
            if (dhi.neighbors[i]) {
                if (encCmp(*(dhi.neighbors[i]), key) <= accpt) {
                    // this is the next hop
                    if (*(dhi.neighbors[i]) != pukeyhash) {
                        return *(dhi.nbors_keys[i]);
                    } else {
                        return "";
                    }
                }
            }
        }
        
        // must be for us
        return "";
    }
}

bool dhtForMe(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route, bool noexact)
{
    if (in_dhts.find(msg.params[2]) == in_dhts.end()) return false;
    BinSeq nextHop = dhtNextHop(msg.params[2], msg.params[3], noexact);
    
    if (nextHop.size()) {
        cout << "Not for me, bounce to ";
        BinSeq h = encHashKey(nextHop);
        outHex(h.c_str(), h.size());
        cout << endl;
        if (dn_routes->find(nextHop) != dn_routes->end()) {
            msg.params[0] = (*dn_routes)[nextHop]->toBinSeq();
            
            // append it to the current route
            if (route) {
                Route *nhop = (*dn_routes)[nextHop];
                Route rroute(*route);
                rroute.append(*nhop);
                
                *route = rroute.toBinSeq();
                cout << "Route to them:" << endl;
                nhop->debug();
                cout << "Full route:" << endl;
                rroute.debug();
            }
            
            handleRoutedMsg(msg);
        } else {
            cout << "ACK!  No route!" << endl;
        }
        return false;
    }
    
    return true;
}

void handleDHTDupMessage(conn_t *conn, Message msg)
{
    handleDHTMessage(conn, msg);
}

#define REQ_PARAMS(x) if (msg.params.size() < x) return
#define CMD_IS(x, y, z) (msg.cmd == x && msg.ver[0] == y && msg.ver[1] == z)

void handleDHTMessage(conn_t *conn, Message &msg)
{
    //msg.debug("DHT");
    
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
                    dhtEstablish((*dhtl)[i]);
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
        
        if (msg.params[6].size() < 2) return;
        
        if (!dhtForMe(msg, msg.params[2], msg.params[3], &(msg.params[4]),
                      (msg.params[6][0] == '\x04'))) return;
        
        DHTInfo &indht = in_dhts[msg.params[2]];
            
        // we're the destination, what kind of search is it?
        switch (msg.params[6][0]) {
            case '\x03':
            {
                // search for neighbors
                BinSeq neighbors[4];
                    
                // set their keys
                if (indht.nbors_keys[1]) neighbors[0] = *(indht.nbors_keys[1]);
                neighbors[1] = encExportKey();
                if (indht.nbors_keys[2]) neighbors[2] = *(indht.nbors_keys[2]);
                if (indht.nbors_keys[3]) neighbors[3] = *(indht.nbors_keys[3]);
                        
                // add them to our DHT
                if (indht.nbors_keys[3]) delete indht.nbors_keys[3];
                indht.nbors_keys[3] = indht.nbors_keys[2];
                indht.nbors_keys[2] = new BinSeq(msg.params[5]);
                    
                if (indht.neighbors[3]) delete indht.neighbors[3];
                indht.neighbors[3] = indht.neighbors[2];
                indht.neighbors[2] = new BinSeq(encHashKey(msg.params[5]));
                    
                // get the return route
                Route toroute(msg.params[4]);
                Route rroute(toroute);
                // this route has us included, and doesn't have the end user
                if (rroute.size()) rroute.pop_back();
                rroute.reverse();
                rroute.push_back(encHashKey(msg.params[5]));
                BinSeq brroute = rroute.toBinSeq();
                    
                // add it to our routes
                if (dn_routes->find(msg.params[5]) == dn_routes->end()) {
                    (*dn_routes)[msg.params[5]] = new Route(rroute);
                }
                    
                // prepare to make other routes
                Route *subRoute;
                    
                // send our four-part response
                Message msgb(1, "Hfr", 1, 1);
                msgb.params.push_back(brroute);
                msgb.params.push_back(dn_name);
                msgb.params.push_back(indht.HTI);
                if (neighbors[0] == pukeyhash) {
                    // route back to me
                    msgb.params.push_back(toroute.toBinSeq());
                } else if (dn_routes->find(neighbors[0]) != dn_routes->end()) {
                    subRoute = new Route(toroute);
                    subRoute->append(*((*dn_routes)[neighbors[0]]));
                    msgb.params.push_back(subRoute->toBinSeq());
                    delete subRoute;
                } else {
                    msgb.params.push_back(Route().toBinSeq());
                }
                msgb.params.push_back(neighbors[0]);
                msgb.params.push_back(BinSeq("\x03\x00", 2));
                handleRoutedMsg(msgb);
                    
                for (int i = 1; i < 4; i++) {
                    if (neighbors[i] == pukeyhash) {
                        // route back to me
                        msgb.params[3] = toroute.toBinSeq();
                    } else if (dn_routes->find(neighbors[i]) != dn_routes->end()) {
                        subRoute = new Route(toroute);
                        subRoute->append(*((*dn_routes)[neighbors[i]]));
                        msgb.params[3] = subRoute->toBinSeq();
                        delete subRoute;
                    } else {
                        msgb.params[3] = Route().toBinSeq();
                    }
                    msgb.params[4] = neighbors[i];
                    msgb.params[5][0] = '\x03' + i;
                    handleRoutedMsg(msgb);
                }
                    
                // then update affected neighbors
                if (indht.neighbors[3]) {
                    if (*(indht.neighbors[3]) == pukeyhash) {
                        msgb.params[3] = Route().toBinSeq();
                        msgb.params[4] = encExportKey();
                        msgb.params[5][0] = '\x03';
                        handleDHTDupMessage(conn, msgb);
                            
                        msgb.params[3] = brroute;
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x04';
                        handleDHTDupMessage(conn, msgb);
                    } else if (dn_routes->find(*(indht.nbors_keys[3])) != dn_routes->end()) {
                        Route rt(*((*dn_routes)[*(indht.nbors_keys[3])]));
                        BinSeq nroute = rt.toBinSeq(); // route to them
                            
                        // return route
                        if (rt.size()) rt.pop_back();
                        rt.reverse();
                        rt.push_back(pukeyhash);
                            
                        msgb.params[0] = nroute;
                        msgb.params[3] = rt.toBinSeq();
                        msgb.params[4] = encExportKey();
                        msgb.params[5][0] = '\x03';
                        handleRoutedMsg(msgb);
                            
                        // route to new user
                        rt.append(rroute);
                            
                        msgb.params[3] = rt.toBinSeq();
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x04';
                        handleRoutedMsg(msgb);
                    }
                }
                if (indht.neighbors[1]) {
                    if (*(indht.neighbors[1]) == pukeyhash) {
                        msgb.params[3] = brroute;
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x06';
                        handleDHTDupMessage(conn, msgb);
                    } else if (dn_routes->find(*(indht.nbors_keys[1])) != dn_routes->end()) {
                        Route rt(*((*dn_routes)[*(indht.nbors_keys[1])]));
                        BinSeq nroute = rt.toBinSeq(); // route to them
                            
                        // return route
                        if (rt.size()) rt.pop_back();
                        rt.reverse();
                        rt.push_back(pukeyhash);
                        rt.append(rroute);
                            
                        msgb.params[0] = nroute;
                        msgb.params[3] = rt.toBinSeq();
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x06';
                        handleRoutedMsg(msgb);
                    }
                }
                if (indht.neighbors[3]) {
                    if (*(indht.neighbors[3]) == pukeyhash) {
                        msgb.params[3] = brroute;
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x07';
                        handleDHTDupMessage(conn, msgb);
                    } else if (dn_routes->find(*(indht.nbors_keys[3])) != dn_routes->end()) {
                        Route rt(*((*dn_routes)[*(indht.nbors_keys[3])]));
                        BinSeq nroute = rt.toBinSeq(); // route to them
                            
                        // return route
                        if (rt.size()) rt.pop_back();
                        rt.reverse();
                        rt.push_back(pukeyhash);
                        rt.append(rroute);
                            
                        msgb.params[0] = nroute;
                        msgb.params[3] = rt.toBinSeq();
                        msgb.params[4] = *(indht.nbors_keys[2]);
                        msgb.params[5][0] = '\x07';
                        handleRoutedMsg(msgb);
                    }
                }
            }
                
            case '\x04':
            {
                // reporting a node dead - the dead node is our successor
                if (indht.neighbors[2]) delete indht.neighbors[2];
                indht.neighbors[2] = indht.neighbors[3];
                indht.neighbors[3] = NULL;
                
                if (indht.nbors_keys[2]) delete indht.nbors_keys[2];
                indht.nbors_keys[2] = indht.nbors_keys[3];
                indht.nbors_keys[3] = NULL;
                
                Message msgb(1, "Hfr", 1, 1);
                msgb.params.push_back(Route().toBinSeq());
                msgb.params.push_back(dn_name);
                msgb.params.push_back(indht.HTI);
                msgb.params.push_back(Route().toBinSeq());
                msgb.params.push_back("");
                msgb.params.push_back(BinSeq("\x00\x00", 2));
                
                // update affected neighbors
                if (indht.neighbors[2]) {
                    if (dn_routes->find(*(indht.nbors_keys[2])) != dn_routes->end()) {
                        // route to the neighbor ...
                        Route rt(*((*dn_routes)[*(indht.nbors_keys[2])]));
                        BinSeq brt = rt.toBinSeq();
                        
                        // route to their new predecessor (me)
                        if (rt.size()) rt.pop_back();
                        rt.reverse();
                        rt.push_back(pukeyhash);
                        BinSeq rtm = rt.toBinSeq();
                        
                        msgb.params[0] = brt;
                        msgb.params[3] = rtm;
                        msgb.params[4] = pukeyhash;
                        msgb.params[5][0] = '\x04';
                        handleRoutedMsg(msgb);
                        
                        // route to their new second predecessor
                        if (indht.neighbors[1]) {
                            if (*(indht.neighbors[1]) != pukeyhash &&
                                dn_routes->find(*(indht.nbors_keys[1])) != dn_routes->end()) {
                                rt.append(*((*dn_routes)[*(indht.nbors_keys[1])]));
                            }
                            
                            msgb.params[3] = rt.toBinSeq();
                            msgb.params[4] = *(indht.neighbors[1]);
                            msgb.params[5][0] = '\x03';
                            handleRoutedMsg(msgb);
                        }
                    }
                }
                
                // TODO: finish
            }
        }
        /* TODO: complete */
        
        
    } else if (CMD_IS("Hfr", 1, 1)) {
        REQ_PARAMS(5);
        
        if (in_dhts.find(msg.params[2]) == in_dhts.end()) return;
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        if (msg.params[5].size() < 2) return;
        
        // add the route
        if (msg.params[3].size() > 1) {
            if (dn_routes->find(msg.params[4]) == dn_routes->end()) {
                (*dn_routes)[msg.params[4]] = new Route(msg.params[3]);
            }
        }
        
        if (msg.params[5][0] >= '\x03' &&
            msg.params[5][0] <= '\x06') {
            int nnum = msg.params[5][0] - 3;
            
            if (indht.nbors_keys[nnum])
                delete indht.nbors_keys[nnum];
            indht.nbors_keys[nnum] = new BinSeq(msg.params[4]);
                
            if (indht.neighbors[nnum])
                delete indht.neighbors[nnum];
            indht.neighbors[nnum] = new BinSeq(encHashKey(msg.params[4]));
        } else if (msg.params[5][0] == '\x07') {
            // tell our successor who their second predecessor is
            if (indht.neighbors[2]) {
                if (*(indht.neighbors[2]) == pukeyhash) {
                    // gee, this is us!
                    msg.params[5][0] = '\x03';
                    handleDHTDupMessage(conn, msg);
                } else if (dn_routes->find(*(indht.nbors_keys[2])) != dn_routes->end()) {
                    // continued route
                    Route croute(*((*dn_routes)[*(indht.nbors_keys[2])]));
                    if (croute.size()) croute.pop_back();
                    croute.reverse();
                    croute.push_back(pukeyhash);
                    croute.append(Route(msg.params[3]));
                
                    msg.params[0] = (*dn_routes)[*(indht.nbors_keys[2])]->toBinSeq();
                    msg.params[3] = croute.toBinSeq();
                    msg.params[5][0] = '\x03';
                    handleRoutedMsg(msg);
                }
            }
        }
        /* TODO: complete */
        
        dhtEstablish(msg.params[2]);
    }
    
    map<BinSeq, DHTInfo>::iterator dhti;
    for (dhti = in_dhts.begin(); dhti != in_dhts.end(); dhti++) {
        dhtDebug(dhti->first);
    }
}
