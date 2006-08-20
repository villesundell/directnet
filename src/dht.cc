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
    
    // set our name into the hash
    BinSeq hname = "nm:" + string(dn_name);
    Message nmsg(1, "Had", 1, 1);
    nmsg.params.push_back(Route().toBinSeq());
    nmsg.params.push_back(dn_name);
    nmsg.params.push_back(ident);
    nmsg.params.push_back(hname);
    nmsg.params.push_back(pukeyhash);
    nmsg.params.push_back(" ");
    dhtSendMsg(nmsg, ident, encHashKey(hname), NULL);
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

void outHex(const char *dat, int count)
{
    for (int i = 0; i < count; i++) {
        printf("%.2X", ((const unsigned char *) dat)[i]);
    }
}

void dhtEstablish(const BinSeq &ident, int step)
{
    if (in_dhts.find(ident) == in_dhts.end()) return;
    
    DHTInfo &di = in_dhts[ident];
    
    if (!di.rep) return; // yukk!
    if ((step == -1 && !di.neighbors[0]) ||
        step == DHT_ESTABLISH_NEIGHBORS) {
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
        msg.params.push_back(BinSeq("\x03\x00", 2));
        
        handleRoutedMsg(msg);
        
        // TODO: continue
    } else if (di.neighbors[0] && di.neighbors[1] && di.neighbors[2] && di.neighbors[3]) {
        // check divisions
        int ldiv = di.real_divisions.size() - 1;
        if (ldiv < 0 || 
            (di.real_divisions[ldiv] &&
             *(di.real_divisions[ldiv]) != pukeyhash)) {
            // more divisions!
            ldiv++;
            BinSeq off = encOffset(ldiv);
            di.best_divisions.push_back(new BinSeq(off));
            di.real_divisions.push_back(NULL);
            di.real_divs_keys.push_back(NULL);
            
            // make the find messages
            Message msg(1, "Hfn", 1, 1);
            msg.params.push_back(Route().toBinSeq());
            msg.params.push_back(dn_name);
            msg.params.push_back(di.HTI);
            msg.params.push_back(off);
            msg.params.push_back(Route().toBinSeq());
            msg.params.push_back(encExportKey());
            msg.params.push_back(BinSeq("\x01\x00", 2));
            msg.params[6][1] = (unsigned char) ldiv;
            dhtSendMsg(msg, di.HTI, off, &(msg.params[4]));
            
            // a reverse find
            BinSeq roff = encOffset(ldiv, true);
            msg.params[3] = roff;
            msg.params[6][0] = '\x02';
            dhtSendMsg(msg, di.HTI, roff, &(msg.params[4]));
            
        } else {
            // this is already established!
            di.established = true;
        }
    }
}

#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

void dhtDebug(const BinSeq &ident)
{
    if (in_dhts.find(ident) == in_dhts.end()) return;
    
    DHTInfo &di = in_dhts[ident];
    
    /*cout << "Key: ";
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
    
    cout << "--------------------------------------------------" << endl << endl;*/
    
    Route info;
    info.push_back(pukeyhash);
    if (di.neighbors[2]) {
        info.push_back(*(di.neighbors[2]));
    } else {
        info.push_back(Route().toBinSeq());
    }
    
    for (int i = 0; i < 4; i++) {
        if (i == 2) continue;
        if (di.neighbors[i]) {
            info.push_back(*(di.neighbors[i]));
        }
    }
    
    for (int i = 0; i < di.real_divisions.size(); i++) {
        if (di.real_divisions[i]) {
            info.push_back(*(di.real_divisions[i]));
        }
    }
    
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    int len, i;
    FILE *f;
    
    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_port = 0;
    si.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(fd, (struct sockaddr *) &si, sizeof(si));
    
    struct sockaddr_in so;
    so.sin_family = AF_INET;
    so.sin_port = htons(3447);
    
    struct hostent *he = gethostbyname("localhost");
    so.sin_addr = *((struct in_addr *) he->h_addr);
    
    BinSeq out = info.toBinSeq();
    sendto(fd, out.c_str(), out.size(), 0, (struct sockaddr *) &so, sizeof(so));
    printf("Sent DHT info.\n");
    
    close(fd);
}

void dhtCheck()
{
    return;
    
    // for all DHTs ...
    map<BinSeq, DHTInfo>::iterator dhi;
    for (dhi = in_dhts.begin(); dhi != in_dhts.end(); dhi++) {
        DHTInfo &indht = dhi->second;
        
        // check neighbors...
        for (int i = 0; i < 4; i++) {
            if (indht.neighbors[i]) {
                if (dn_routes->find(*(indht.nbors_keys[i])) == dn_routes->end()) {
                    // uh oh!
                    indht.established = false;
                    
                    delete indht.neighbors[i];
                    indht.neighbors[i] = NULL;
                    
                    delete indht.nbors_keys[i];
                    indht.nbors_keys[i] = NULL;
                    
                    dhtEstablish(indht.HTI, DHT_ESTABLISH_NEIGHBORS);
                }
            }
        }
        
        // TODO: check divisions
    }
}

BinSeq dhtNextHop(const BinSeq &ident, const BinSeq &key, bool closer)
{
    if (in_dhts.find(ident) == in_dhts.end()) return "";
    DHTInfo &dhi = in_dhts[ident];
    
    if (!dhi.established) {
        if (!dhi.neighbors[2]) {
            // just send it to the rep
            if (!dhi.rep) return "";
            return *(dhi.rep);
        }
    }
    
    for (int i = 0; i < dhi.real_divisions.size(); i++) {
        if (dhi.real_divisions[i]) {
            if ((!closer && encCmp(*(dhi.real_divisions[i]), key) <= 0) ||
                ( closer && encCloser(*(dhi.real_divisions[i]), key))) {
                // this is the next hop
                if (*(dhi.real_divisions[i]) != pukeyhash) {
                    return *(dhi.real_divs_keys[i]);
                }
            }
        }
    }
    
    // check neighbors in case the divisions aren't up yet
    for (int i = 2; i >= 1; i--) {
        if (dhi.neighbors[i]) {
            if ((!closer && encCmp(*(dhi.neighbors[i]), key) <= 0) ||
                ( closer && encCloser(*(dhi.neighbors[i]), key))) {
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

bool dhtForMe(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route, bool closer)
{
    if (in_dhts.find(msg.params[2]) == in_dhts.end()) return false;
    BinSeq nextHop = dhtNextHop(msg.params[2], msg.params[3], closer);
    
    if (nextHop.size()) {
        if (dn_routes->find(nextHop) != dn_routes->end()) {
            msg.params[0] = (*dn_routes)[nextHop]->toBinSeq();
            
            // append it to the current route
            if (route) {
                Route *nhop = (*dn_routes)[nextHop];
                Route rroute(*route);
                rroute.append(*nhop);
                
                *route = rroute.toBinSeq();
            }
            
            handleRoutedMsg(msg);
        }
        return false;
    }
    
    return true;
}

void dhtSendMsg(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route)
{
    if (dhtForMe(msg, ident, key, route)) {
        handleDHTDupMessage(NULL, msg);
    }
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
    
    if (CMD_IS("Had", 1, 1)) {
        REQ_PARAMS(6);
        return;
        
        if (!dhtForMe(msg, msg.params[2], encHashKey(msg.params[3]), NULL))
            return;
        
        // this is my data, store it
        // TODO: data ownership, redundancy, etc
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        indht.data[msg.params[3]].insert(msg.params[4]);
        cout << msg.params[3].c_str() << " = " << msg.params[4].c_str() << endl;
        
        // now send the redundant info
        if (indht.neighbors[1] &&
            dn_routes->find(*(indht.nbors_keys[1])) != dn_routes->end()) {
            Route *toroute = (*dn_routes)[*(indht.nbors_keys[1])];
            
            // make the outgoing data
            Route rdat;
            set<BinSeq>::iterator di;
            for (di = indht.data[msg.params[3]].begin();
                 di != indht.data[msg.params[3]].end();
                 di++) {
                rdat.push_back(*di);
            }
            
            Message rdmsg(1, "Hrd", 1, 1);
            rdmsg.params.push_back(toroute->toBinSeq());
            rdmsg.params.push_back(dn_name);
            rdmsg.params.push_back(indht.HTI);
            rdmsg.params.push_back(msg.params[3]);
            rdmsg.params.push_back(rdat.toBinSeq());
            rdmsg.params.push_back(BinSeq());
            handleRoutedMsg(rdmsg);
        }
            
            
    } else if (CMD_IS("Hga", 1, 1)) {
        REQ_PARAMS(6);
        
        if (!dhtForMe(msg, msg.params[2], encHashKey(msg.params[3]), &(msg.params[4])))
            return;
        
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        // I should have this data, so return it
        Route retdata;
        
        if (indht.data.find(msg.params[3]) != indht.data.end()) {
            set<BinSeq> &reqd = indht.data[msg.params[3]];
            set<BinSeq>::iterator ri;
            for (ri = reqd.begin(); ri != reqd.end(); ri++) {
                retdata.push_back(*ri);
            }
        }
        
        // build the return message
        Message ret(1, "Hra", 1, 1);
        
        Route rroute(msg.params[0]);
        if (rroute.size()) rroute.pop_back();
        rroute.reverse();
        rroute.push_back(encHashKey(msg.params[5]));
        ret.params.push_back(rroute.toBinSeq());
        
        ret.params.push_back(dn_name);
        ret.params.push_back(indht.HTI);
        ret.params.push_back(msg.params[3]);
        ret.params.push_back(retdata.toBinSeq());
        
        handleRoutedMsg(ret);
        
        
    } else if (CMD_IS("Hfn", 1, 1)) {
        // find
        REQ_PARAMS(7);
        
        if (msg.params[6].size() < 2) return;
        
        if (!dhtForMe(msg, msg.params[2], msg.params[3], &(msg.params[4]),
                      (msg.params[6][0] == '\x01' || msg.params[6][0] == '\x02'))) return;
        
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        // we're the destination, what kind of search is it?
        switch (msg.params[6][0]) {
            case '\x00':
            {
                // regular search - if we don't match exactly, failed search!
                if (pukeyhash != msg.params[3]) return;
                
                Route *fromroute = new Route(msg.params[4]);
                Route *rroute = new Route(*fromroute);
                // rroute needs to be reversed
                if (rroute->size()) rroute->pop_back();
                rroute->reverse();
                rroute->push_back(encHashKey(msg.params[5]));
                
                recvFnd(rroute, msg.params[1], msg.params[5]);
                
                // then send the Hfr back
                Message msgb(1, "Hfr", 1, 1);
                msgb.params.push_back(rroute->toBinSeq());
                delete rroute;
                msgb.params.push_back(dn_name);
                msgb.params.push_back(indht.HTI);
                msgb.params.push_back(fromroute->toBinSeq());
                delete fromroute;
                msgb.params.push_back(encExportKey());
                msgb.params.push_back(BinSeq("\x00\x00", 2));
                handleRoutedMsg(msgb);
                
                break;
            }
            
            case '\x01':
            {
                // search for positive divisions, simply echo
                
                // get the return route
                Route toroute(msg.params[4]);
                Route rroute(toroute);
                // this route has us included, and doesn't have the end user
                if (rroute.size()) rroute.pop_back();
                rroute.reverse();
                rroute.push_back(encHashKey(msg.params[5]));
                
                Message msgb(1, "Hfr", 1, 1);
                msgb.params.push_back(rroute.toBinSeq());
                msgb.params.push_back(dn_name);
                msgb.params.push_back(indht.HTI);
                msgb.params.push_back(msg.params[4]);
                msgb.params.push_back(encExportKey());
                msgb.params.push_back(BinSeq("\x01\x00", 2));
                msgb.params[5][1] = msg.params[6][1];
                if (msg.params[5] == encExportKey()) {
                    // it's for me!
                    handleDHTDupMessage(conn, msgb);
                } else {
                    handleRoutedMsg(msgb);
                }
                break;
            }
            
            case '\x02':
            {
                // search for negative divisions, add info (no echo)
                while (msg.params[6][1] >= indht.real_divisions.size()) {
                    indht.best_divisions.push_back(NULL); // BAD
                    indht.real_divisions.push_back(NULL);
                    indht.real_divs_keys.push_back(NULL);
                }
                int dnum = msg.params[6][1];
                
                // add the route
                Route rroute(msg.params[4]);
                if (rroute.size()) rroute.pop_back();
                rroute.reverse();
                rroute.push_back(encHashKey(msg.params[5]));
                dn_addRoute(msg.params[5], rroute);
                
                // and the info
                if (indht.real_divs_keys[dnum]) delete indht.real_divs_keys[dnum];
                indht.real_divs_keys[dnum] = new BinSeq(msg.params[5]);
                if (indht.real_divisions[dnum]) delete indht.real_divisions[dnum];
                indht.real_divisions[dnum] = new BinSeq(encHashKey(msg.params[5]));
                
                dhtEstablish(indht.HTI);
                break;
            }
            
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
                dn_addRoute(msg.params[5], rroute);
                
                // prepare to make other routes
                Route *subRoute;
                    
                // send our four-part response
                Message msgb(1, "Hfr", 1, 1);
                msgb.params.push_back(brroute);
                msgb.params.push_back(dn_name);
                msgb.params.push_back(indht.HTI);
                if (neighbors[0] == encExportKey()) {
                    // route back to me
                    msgb.params.push_back(toroute.toBinSeq());
                } else if (dn_routes->find(neighbors[0]) != dn_routes->end()) {
                    subRoute = new Route(toroute);
                    subRoute->append(*((*dn_routes)[neighbors[0]]));
                    msgb.params.push_back(subRoute->toBinSeq());
                    delete subRoute;
                } else {
                    printf("NO ROUTE TO ");
                    outHex(neighbors[0].c_str(), neighbors[0].size());
                    printf("\n");
                    msgb.params.push_back(Route().toBinSeq());
                }
                msgb.params.push_back(neighbors[0]);
                msgb.params.push_back(BinSeq("\x03\x00", 2));
                handleRoutedMsg(msgb);
                    
                for (int i = 1; i < 4; i++) {
                    if (neighbors[i] == encExportKey()) {
                        // route back to me
                        msgb.params[3] = toroute.toBinSeq();
                    } else if (dn_routes->find(neighbors[i]) != dn_routes->end()) {
                        subRoute = new Route(toroute);
                        subRoute->append(*((*dn_routes)[neighbors[i]]));
                        msgb.params[3] = subRoute->toBinSeq();
                        delete subRoute;
                    } else {
                        printf("NO ROUTE TO ");
                        outHex(neighbors[i].c_str(), neighbors[0].size());
                        printf("\n");
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
                    } else {
                        printf("NO ROUTE TO ");
                        outHex(indht.neighbors[3]->c_str(), indht.neighbors[3]->size());
                        printf("\n");
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
                    } else {
                        printf("NO ROUTE TO ");
                        outHex(indht.neighbors[1]->c_str(), indht.neighbors[1]->size());
                        printf("\n");
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
                    } else {
                        printf("NO ROUTE TO ");
                        outHex(indht.neighbors[3]->c_str(), indht.neighbors[3]->size());
                        printf("\n");
                    }
                }
                
                break;
            }
        }
        /* TODO: complete */
        
        
    } else if (CMD_IS("Hfr", 1, 1)) {
        REQ_PARAMS(6);
        
        if (in_dhts.find(msg.params[2]) == in_dhts.end()) return;
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        if (msg.params[5].size() < 2) return;
        
        // add the route
        if (msg.params[3].size() > 1) {
            Route rroute(msg.params[3]);
            dn_addRoute(msg.params[4], msg.params[3]);
        }
        
        if (msg.params[5][0] == '\x00') {
            // normal search
            Route *rt = new Route(msg.params[3]);
            recvFnd(rt, msg.params[1], msg.params[4]);
            delete rt;
            
        } else if (msg.params[5][0] == '\x01') {
            // positive division
            if (msg.params[5][1] > indht.real_divisions.size()) return; // fake!
            int dnum = msg.params[5][1];
            
            // add this info
            if (indht.real_divs_keys[dnum]) delete indht.real_divs_keys[dnum];
            indht.real_divs_keys[dnum] = new BinSeq(msg.params[4]);
            if (indht.real_divisions[dnum]) delete indht.real_divisions[dnum];
            indht.real_divisions[dnum] = new BinSeq(encHashKey(msg.params[4]));
            
        } else if (msg.params[5][0] >= '\x03' &&
                   msg.params[5][0] <= '\x06') {
            // neighbors
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
                } else {
                    printf("NO ROUTE TO ");
                    outHex(indht.neighbors[2]->c_str(), indht.neighbors[2]->size());
                    printf("\n");
                }
            }
        }
        /* TODO: complete */
        
        dhtEstablish(msg.params[2]);
        
        
    } else if (CMD_IS("Hin", 1, 1) ||
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
        
        
    } else if (CMD_IS("Hra", 1, 1)) {
        REQ_PARAMS(5);
        
        /* FIXME: right now, this assumes that I actually performed this search
         * and want to find this user :) */
        if (msg.params[3].substr(0, 2) == "n:") {
            // build the find for this user (these users?)
            Route vals(msg.params[4]);
            Route::iterator vi;
            
            for (vi = vals.begin(); vi != vals.end(); vi++) {
                Message fms(1, "Hfn", 1, 1);
                fms.params.push_back(Route().toBinSeq());
                fms.params.push_back(dn_name);
                fms.params.push_back(msg.params[2]);
                fms.params.push_back(encHashKey(*vi));
                fms.params.push_back(Route().toBinSeq());
                fms.params.push_back(encExportKey());
                fms.params.push_back(BinSeq("\x00\x00", 2));
                
                dhtSendMsg(fms, fms.params[2], fms.params[3], &(fms.params[4]));
            }
        }
        
        
    } else if (CMD_IS("Hrd", 1, 1)) {
        REQ_PARAMS(6);
        
        // store redundant info
        // FIXME: verification
        if (in_dhts.find(msg.params[2]) == in_dhts.end()) return;
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        Route dat(msg.params[4]);
        Route::iterator ri;
        indht.rdata[msg.params[3]].clear();
        for (ri = dat.begin(); ri != dat.end(); ri++) {
            indht.rdata[msg.params[3]].insert(*ri);
            cout << "Redundant: " << msg.params[3].c_str() << " = " << msg.params[4].c_str() << endl;
        }
        
        
    }
    
    map<BinSeq, DHTInfo>::iterator dhti;
    for (dhti = in_dhts.begin(); dhti != in_dhts.end(); dhti++) {
        dhtDebug(dhti->first);
    }
}
