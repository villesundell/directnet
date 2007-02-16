/*
 * Copyright 2006, 2007  Gregor Richards
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
using namespace std;

extern "C" {
#include <string.h>
}

#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dnconfig.h"
#include "dn_event.h"
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
    estingNeighbors = false;
}

map<BinSeq, DHTSearchInfo> dhtSearches;

map<BinSeq, fndCallbackWData> dhtFnds;

Route dhtIn(bool must)
{
    Route toret;
    
    // if we need one, make three (redundancy)
    if (must && in_dhts.size() == 0) {
        BinSeq hti = pukeyhash;
        dhtCreate(hti);
        hti = encHashKey(hti);
        dhtCreate(hti);
        hti = encHashKey(hti);
        dhtCreate(hti);
    }
    
    // iterate through all that we're in
    map<BinSeq, DHTInfo>::iterator di;
    
    for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
        toret.push_back(di->first);
    }
    
    return toret;
}

/* Age the data in a DHT (on a timer, payload == DHT) */
void dhtDataAge(dn_event_timer *te)
{
    te->setTimeDelta(60, 0);
    DHTInfo &indht = *((DHTInfo *) te->payload);
    
    // go through each piece of data, aging it
    dhtData_t::iterator dai;
    dhtDataValue_t::iterator dvi;
    
    for (dai = indht.data.begin();
         dai != indht.data.end();
         dai++) {
        for (dvi = dai->second.begin();
             dvi != dai->second.end();
             dvi++) {
            (*(dvi->timeleft))--;
            if (*(dvi->timeleft) == 0) {
                dai->second.erase(dvi);
                dvi--;
            }
        }
        
        if (dai->second.size() == 0) {
            indht.data.erase(dai);
            dai--;
        }
    }
    
    for (dai = indht.rdata.begin();
         dai != indht.rdata.end();
         dai++) {
        for (dvi = dai->second.begin();
             dvi != dai->second.end();
             dvi++) {
            (*(dvi->timeleft))--;
            if (*(dvi->timeleft) == 0) {
                dai->second.erase(dvi);
                dvi--;
            }
        }
        
        if (dai->second.size() == 0) {
            indht.rdata.erase(dai);
            dai--;
        }
    }
}

void dhtJoin(const BinSeq &ident, const BinSeq &rep)
{
    in_dhts.erase(ident);
    DHTInfo &dhti = in_dhts[ident];
    dhti.rep = new BinSeq(rep);
    dhti.HTI = ident;
    
    // send our name into the hash
    BinSeq hname("\x08nm", 3);
    hname += dn_name;
    dhtSendAdd(hname, pukeyhash, &dhti);
    
    // set up the data aging timer
    dn_event_timer *te = new dn_event_timer(
        60, 0, dhtDataAge, &dhti);
    te->activate();
}

void dhtCreate(const BinSeq &key)
{
    // make it
    DHTInfo &newDHT = in_dhts[key];
    
    // establish it
    newDHT.HTI = key;
    newDHT.rep = new BinSeq(encExportKey());
    for (int i = 0; i < 4; i++) {
        newDHT.neighbors[i] = new BinSeq(pukeyhash);
        newDHT.nbors_keys[i] = new BinSeq(encExportKey());
    }
    newDHT.established = true;
    
    // add our own data with refresher loop et all
    BinSeq nminfo = BinSeq("\x08nm", 3) + dn_name;
    dhtSendAdd(nminfo, pukeyhash, &newDHT);
    
    // set up the data aging timer
    dn_event_timer *te = new dn_event_timer(
        60, 0, dhtDataAge, &newDHT);
    te->activate();
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
    if ((step == -1 && !di.neighbors[2]) ||
        step == DHT_ESTABLISH_NEIGHBORS) {
        if (!di.neighbors[0] &&
            !di.neighbors[1] &&
            !di.neighbors[2] &&
            !di.neighbors[3]) {
            di.estingNeighbors = true;
            
            // send a neighbor request to the representative
            Message msg(1, "Hfn", 1, 1);
            BinSeq rt;
            
            if (dn_routes->find(*di.rep) != dn_routes->end()) {
                rt = (*dn_routes)[*di.rep]->toBinSeq();
            } else {
                // the only reasonable thing to do here is assume an empty DHT
                for (int i = 0; i < 4; i++) {
                    di.neighbors[i] = new BinSeq(pukeyhash);
                    di.nbors_keys[i] = new BinSeq(encExportKey());
                }
                return;
            }
        
            msg.params.push_back(rt);
            msg.params.push_back(dn_name);
            msg.params.push_back(di.HTI);
            msg.params.push_back(pukeyhash);
            msg.params.push_back(rt);
            msg.params.push_back(encExportKey());
            msg.params.push_back(BinSeq("\x03\x00", 2));
        
            handleRoutedMsg(msg);
        } else if (!di.estingNeighbors) {
            BinSeq lost;
            
            if (!di.neighbors[1]) {
                // missing our immediate predecessor, pull in the second
                if (di.nbors_keys[1]) {
                    lost = *(di.nbors_keys[1]);
                    delete di.nbors_keys[1];
                }
                di.neighbors[1] = di.neighbors[0];
                di.neighbors[0] = NULL;
                di.nbors_keys[1] = di.nbors_keys[0];
                di.nbors_keys[0] = NULL;
            }
            if (!di.neighbors[0]) {
                if (di.nbors_keys[0]) {
                    lost = *(di.nbors_keys[0]);
                    delete di.nbors_keys[0];
                    di.nbors_keys[0] = NULL;
                }
                if (di.neighbors[1]) {
                    // ask our predecessor who their predecessor is
                    Message msg(1, "Hgn", 1, 1);
                    Route *toroute, rroute;
                    if (*(di.neighbors[1]) != pukeyhash &&
                        dn_routes->find(*(di.nbors_keys[1])) != dn_routes->end()) {
                        toroute = (*dn_routes)[*(di.nbors_keys[1])];
                        msg.params.push_back(toroute->toBinSeq());
                        rroute = *toroute;
                    } else {
                        msg.params.push_back(Route().toBinSeq());
                        rroute = Route();
                    }
                    
                    msg.params.push_back(dn_name);
                    msg.params.push_back(di.HTI);
                    
                    // add also the return route
                    if (rroute.size()) {
                        rroute.pop_back();
                        rroute.reverse();
                        rroute.push_back(pukeyhash);
                    }
                    msg.params.push_back(rroute.toBinSeq());
                    
                    msg.params.push_back(lost);
                    
                    msg.params.push_back(BinSeq("\x00", 1));
                    
                    if (*(di.neighbors[1]) == pukeyhash) {
                        handleDHTDupMessage(NULL, msg);
                    } else {
                        handleRoutedMsg(msg);
                    }
                } else {
                    // VERY BAD
                    //printf("I've lost the DHT :(\n");
                }
            }
            
            // FIXME: duplication
            if (!di.neighbors[2]) {
                // missing our immediate predecessor, pull in the second
                if (di.nbors_keys[2]) {
                    lost = *(di.nbors_keys[2]);
                    delete di.nbors_keys[2];
                }
                di.neighbors[2] = di.neighbors[3];
                di.neighbors[3] = NULL;
                di.nbors_keys[2] = di.nbors_keys[3];
                di.nbors_keys[3] = NULL;
            }
            if (!di.neighbors[3]) {
                if (di.nbors_keys[3]) {
                    lost = *(di.nbors_keys[3]);
                    delete di.nbors_keys[3];
                    di.nbors_keys[3] = NULL;
                }
                if (di.neighbors[2]) {
                    // ask our predecessor who their predecessor is
                    Message msg(1, "Hgn", 1, 1);
                    Route *toroute, rroute;
                    if (*(di.neighbors[2]) != pukeyhash &&
                        dn_routes->find(*(di.nbors_keys[2])) != dn_routes->end()) {
                        toroute = (*dn_routes)[*(di.nbors_keys[2])];
                        msg.params.push_back(toroute->toBinSeq());
                        rroute = *toroute;
                    } else {
                        msg.params.push_back(Route().toBinSeq());
                        rroute = Route();
                    }
                    
                    msg.params.push_back(dn_name);
                    msg.params.push_back(di.HTI);
                    
                    // add also the return route
                    if (rroute.size()) {
                        rroute.pop_back();
                        rroute.reverse();
                        rroute.push_back(pukeyhash);
                    }
                    msg.params.push_back(rroute.toBinSeq());
                    
                    msg.params.push_back(lost);
                    
                    msg.params.push_back(BinSeq("\x01", 1));
                    
                    if (*(di.neighbors[2]) == pukeyhash) {
                        handleDHTDupMessage(NULL, msg);
                    } else {
                        handleRoutedMsg(msg);
                    }
                } else {
                    // VERY BAD
                    //printf("I've lost the DHT :(\n");
                }
            }
        }
        
        // TODO: continue
    } else if (di.neighbors[0] && di.neighbors[1] && di.neighbors[2] && di.neighbors[3]) {
        di.estingNeighbors = false;
        
        // check divisions
        int ldiv = di.real_divisions.size();
        
        if (ldiv == 0) {
            // start it off
            ldiv = 1;
            di.best_divisions.push_back(NULL);
            di.real_divisions.push_back(NULL);
            di.real_divs_keys.push_back(NULL);
        }
        
        for (int i = 0; i < ldiv; i++) {
            if (!(di.best_divisions[i]) && !(di.real_divisions[i])) {
                // establish this division
                BinSeq off;
                if (!(di.best_divisions[i])) {
                    off = encOffset(i);
                    di.best_divisions[i] = new BinSeq(off);
                } else {
                    off = *(di.best_divisions[i]);
                }
                
                if (di.real_divisions[i]) delete di.real_divisions[i];
                di.real_divisions[i] = NULL;
                
                if (di.real_divs_keys[i]) delete di.real_divs_keys[i];
                di.real_divs_keys[i] = NULL;
                
                // make the find messages
                Message msg(1, "Hfn", 1, 1);
                msg.params.push_back(Route().toBinSeq());
                msg.params.push_back(dn_name);
                msg.params.push_back(di.HTI);
                msg.params.push_back(off);
                msg.params.push_back(Route().toBinSeq());
                msg.params.push_back(encExportKey());
                msg.params.push_back(BinSeq("\x01\x00", 2));
                msg.params[6][1] = (unsigned char) i;
                dhtSendMsg(msg, di.HTI, off, &(msg.params[4]));
                
                // a reverse find
                BinSeq roff = encOffset(i, true);
                msg.params[3] = roff;
                msg.params[6][0] = '\x02';
                dhtSendMsg(msg, di.HTI, roff, &(msg.params[4]));
                
                return;
            } else if (i == ldiv - 1 &&
                       di.real_divisions[i] &&
                       *(di.real_divisions[i]) != pukeyhash) {
                // need at least one more
                ldiv++;
                di.best_divisions.push_back(NULL);
                di.real_divisions.push_back(NULL);
                di.real_divs_keys.push_back(NULL);
            }
        }
        
        // this is already established!
        if (!di.established) {
            di.established = true;
            
            // upon establishment, send finds
            autoFind();
        }
    }
}

/*#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>*/

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
    
    cout << "  Data:" << endl;
    dhtData_t::iterator dati;
    for (dati = di.data.begin(); dati != di.data.end(); dati++) {
        cout << "    " << dati->first.c_str() << endl;
        BinSeq hash = encHashKey(dati->first);
        cout << "      ";
        outHex(hash.c_str(), hash.size());
        cout << endl;
    }
    
    cout << "  Redundant data:" << endl;
    for (dati = di.rdata.begin(); dati != di.rdata.end(); dati++) {
        cout << "    " << dati->first.c_str() << endl;
        BinSeq hash = encHashKey(dati->first);
        cout << "      ";
        outHex(hash.c_str(), hash.size());
        cout << endl;
    }
    
    cout << "--------------------------------------------------" << endl << endl;
    
    /*Route info;
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
    //printf("Sent DHT info.\n");
    
    close(fd);*/
}

void dhtCheck()
{
    // for all DHTs ...
    map<BinSeq, DHTInfo>::iterator dhi;
    for (dhi = in_dhts.begin(); dhi != in_dhts.end(); dhi++) {
        DHTInfo &indht = dhi->second;
        bool divreest = false;
        int step = -1;
        
        // check neighbors...
        for (int i = 0; i < 4; i++) {
            if (indht.neighbors[i]) {
                if (dn_routes->find(*(indht.nbors_keys[i])) == dn_routes->end()) {
                    // uh oh!
                    indht.established = false;
                    
                    /*outHex(pukeyhash.c_str(), pukeyhash.size());
                    printf(" lost ");
                    outHex(indht.neighbors[i]->c_str(), indht.neighbors[i]->size());
                    printf("\n");*/
                    
                    delete indht.neighbors[i];
                    indht.neighbors[i] = NULL;
                    
                    // leave the key for Hgn (see file DHT)
                    
                    divreest = true;
                    step = DHT_ESTABLISH_NEIGHBORS;
                }
            }
        }
        
        // check divisions...
        for (int i = 0; i < indht.real_divisions.size(); i++) {
            if (indht.real_divisions[i]) {
                if (dn_routes->find(*(indht.real_divs_keys[i])) == dn_routes->end()) {
                    // uh oh!
                    indht.established = false;
                    
                    if (indht.best_divisions[i]) {
                        delete indht.best_divisions[i];
                        indht.best_divisions[i] = NULL;
                    }
                    
                    delete indht.real_divisions[i];
                    indht.real_divisions[i] = NULL;
                    
                    delete indht.real_divs_keys[i];
                    indht.real_divs_keys[i] = NULL;
                    
                    divreest = true;
                }
            }
        }
        
        if (divreest) {
            dhtEstablish(indht.HTI, step);
            dhtDebug(indht.HTI);
        }
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
    if (in_dhts.find(ident) == in_dhts.end()) return false;
    BinSeq nextHop = dhtNextHop(ident, key, closer);
    
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

void dhtAllSendMsg(Message &msg, BinSeq *ident, const BinSeq &key, BinSeq *route)
{
    map<BinSeq, DHTInfo>::iterator di;
    for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
        *ident = di->first;
        dhtSendMsg(msg, di->first, key, route);
    }
}

/* Search done callback */
void dhtSearchDone(dn_event_timer *dne)
{
    BinSeq *key = (BinSeq *) dne->payload;
    delete dne;
    
    // hopefully we were actually searching for this key
    if (dhtSearches.find(*key) == dhtSearches.end()) {
        delete key;
        return;
    }
    DHTSearchInfo &ds = dhtSearches[*key];
    
    // figure out how many results we received
    typedef map<set<BinSeq >, int> count_t;
    count_t counts;
    searchResults_t::iterator dsi;
    for (dsi = ds.searchResults.begin();
         dsi != ds.searchResults.end();
         dsi++) {
        // vote on this one
        if (counts.find(dsi->second) != counts.end()) {
            counts[dsi->second]++;
        } else {
            counts[dsi->second] = 1;
        }
    }
    
    // now figure out which is the most likely
    count_t::iterator ci;
    const set<BinSeq> *choice = NULL;
    int cmax = 0;
    for (ci = counts.begin(); ci != counts.end(); ci++) {
        if (ci->second > cmax) {
            cmax = ci->second;
            choice = &(ci->first);
        }
    }
    
    // if we haven't made a choice, choose blank
    set<BinSeq> nochoice;
    if (!choice)
        choice = &nochoice;
    
    // call the callback
    ds.callback(*key, *choice, ds.data);
    
    dhtSearches.erase(*key);
    delete key;
}

/* Send a search for data, with a callback for when it comes */
void dhtSendSearch(const BinSeq &key, dhtSearchCallback callback, void *data)
{
    // store the search info
    DHTSearchInfo &dsi = dhtSearches[key];
    dsi.callback = callback;
    dsi.data = data;
    
    // make the search message
    Message msg(1, "Hga", 1, 1);
    msg.params.push_back(Route().toBinSeq());
    msg.params.push_back("");
    msg.params.push_back(key);
    msg.params.push_back("");
    msg.params.push_back(pukeyhash);
    
    // send out the search
    map<BinSeq, DHTInfo>::iterator di;
    for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
        msg.params[1] = di->first;
        dhtSendMsg(msg, di->first, encHashKey(di->first + key), &(msg.params[3]));
    }
    
    // start the clock for 10 seconds
    dn_event_timer *dne =
        new dn_event_timer(10, 0, dhtSearchDone, (void *) new BinSeq(key));
    dne->activate();
}

// the callback for sendFnd
void sendFndCallback(const BinSeq &key, const set<BinSeq> &values, void *data)
{
    // Connect to each of these users
    Message fms(1, "Hfn", 1, 1);
    fms.params.push_back(Route().toBinSeq());
    fms.params.push_back(dn_name);
    fms.params.push_back("");
    fms.params.push_back("");
    fms.params.push_back(Route().toBinSeq());
    fms.params.push_back(encExportKey());
    fms.params.push_back(BinSeq("\x00\x00", 2));
    
    set<BinSeq>::iterator vi;
    for (vi = values.begin(); vi != values.end(); vi++) {
        fms.params[3] = *vi;
        
        dhtAllSendMsg(fms, &(fms.params[2]), fms.params[3], &(fms.params[4]));
    }
}

void sendFnd(const string &toc, fndCallback callback, void *data)
{
    // Find a user by name
    BinSeq nmcode = "\x08nm" + toc;
    
    if (callback) {
        // FIXME: this should expire
        dhtFnds[toc].set(callback, data);
    }
    
    dhtSendSearch(nmcode, sendFndCallback, NULL);
}

/* Send a find by key
 * key: key to search for
 * callback: function to call (or NULL) upon successful find
 * data: data to send to the callback */
void dhtFindKey(const BinSeq &key, fndCallback callback, void *data)
{
    Message fms(1, "Hfn", 1, 1);
    fms.params.push_back("");
    fms.params.push_back(dn_name);
    fms.params.push_back("");
    fms.params.push_back(key);
    fms.params.push_back("");
    fms.params.push_back(encExportKey());
    fms.params.push_back(BinSeq("\x00\x00", 2));
    
    if (callback) {
        // FIXME: this should expire
        dhtFnds[key].set(callback, data);
    }
    
    dhtAllSendMsg(fms, &(fms.params[2]), key, &(fms.params[0]));
}

/* the refresher loop for dhtSendAdd */
void dhtAddRefresh(dn_event_timer *te)
{
    // "Had" (for search)
    Message &msg = *((Message *) te->payload);
    
    // refresh the time
    unsigned int rtime =
        (1 << (msg.params[2][0] & 0xF)) * 3600;
    // 3/4 of the total time
    rtime *= 3;
    rtime /= 4;
    te->setTimeDelta(rtime, 0);
    
    // then refresh the data
    dhtSendMsg(msg, msg.params[1], encHashKey(msg.params[1] + msg.params[2]), NULL);
}

/* Send a properly-formed add message over the DHT, with a refresher loop */
void dhtSendAdd(const BinSeq &key, const BinSeq &value, DHTInfo *dht)
{
    // if no DHT set, run for each
    if (!dht) {
        map<BinSeq, DHTInfo>::iterator di;
        
        for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
            dhtSendAdd(key, value, &(di->second));
        }
        return;
    }
    
    // make the message
    Message *msg = new Message(1, "Had", 1, 1);
    msg->params.push_back("");
    msg->params.push_back(dht->HTI);
    msg->params.push_back(key);
    msg->params.push_back(value);
    msg->params.push_back("");
    msg->params.push_back("");
    
    // make the timer event
    dn_event_timer *te =
        new dn_event_timer(3600, 0, // time will be reset, unimportant
                           dhtAddRefresh,
                           (void *) msg);
    te->activate();
    
    // run the first iteration (this will set the time and send the data)
    dhtAddRefresh(te);
}

/* Send an atomic add/search */
void dhtSendAddSearch(const BinSeq &key, const BinSeq &value,
                      dhtSearchCallback callback, void *data)
{
    // store the search info
    DHTSearchInfo &dsi = dhtSearches[key];
    dsi.callback = callback;
    dsi.data = data;
    
    // make the message
    Message msg(1, "Hag", 1, 1);
    msg.params.push_back("");
    msg.params.push_back("");
    msg.params.push_back(key);
    msg.params.push_back(value);
    msg.params.push_back("");
    msg.params.push_back("");
    msg.params.push_back("");
    msg.params.push_back("");
    
    // send out the search
    map<BinSeq, DHTInfo>::iterator di;
    for (di = in_dhts.begin(); di != in_dhts.end(); di++) {
        msg.params[1] = di->first;
        dhtSendMsg(msg, di->first, encHashKey(di->first + key), &(msg.params[5]));
    }
    
    // start the clock for 10 seconds
    dn_event_timer *dne =
        new dn_event_timer(10, 0, dhtSearchDone, (void *) new BinSeq(key));
    dne->activate();
}

/* We just changed neighbors, update data */
void dhtNeighborUpdateData(DHTInfo &indht, Route &rroute, BinSeq &hashedKey, int nnum)
{
    if (nnum == 1) {
        // our immediate predecessor, give them all our data as redundant
        dhtData_t::iterator di;
        for (di = indht.data.begin(); di != indht.data.end(); di++) {
            Message rmsg(1, "Hrd", 1, 1);
            rmsg.params.push_back(rroute.toBinSeq());
            rmsg.params.push_back(indht.HTI);
            rmsg.params.push_back(di->first);
            rmsg.params.push_back("");
            rmsg.params.push_back("");
            
            // make each outgoing message
            dhtDataValue_t::iterator dvi;
            for (dvi = di->second.begin(); dvi != di->second.end(); dvi++) {
                rmsg.params[3] = dvi->value;
                rmsg.params[4] = intToBinSeq(*(dvi->timeleft));
                handleRoutedMsg(rmsg);
            }
        }
                
    } else if (nnum == 2) {
        /* our immediate successor: either delete our redundant data or
         * make it normal data (important distinction :) ), then perhaps
         * reassign non-redundant data */
        if (indht.neighbors[2]) {
            if (*(indht.neighbors[2]) == pukeyhash ||
                encCmp(hashedKey, *(indht.neighbors[2])) < 0) {
                /* this person is closer, so nobody's lost. We can junk
                 * all our redundant data */
                indht.rdata.clear();
                        
                // and perhaps reassign some data
                dhtData_t::iterator di;
                bool direstart = false;
                for (di = indht.data.begin(); di != indht.data.end(); di++) {
                    if (direstart) {
                        // we need to start over
                        di = indht.data.begin();
                        direstart = false;
                    }
                            
                    BinSeq dataHash = encHashKey(indht.HTI + di->first);
                    if (encCmp(dataHash, hashedKey) > 0) {
                        // this data belongs with this user
                        Message drmsg(1, "Had", 1, 1);
                        drmsg.params.push_back(rroute.toBinSeq());
                        drmsg.params.push_back(indht.HTI);
                        drmsg.params.push_back(di->first);
                        drmsg.params.push_back("");
                        drmsg.params.push_back("");
                        drmsg.params.push_back("");
                                
                        // add data one-by-one
                        dhtDataValue_t::iterator si;
                        for (si = di->second.begin(); si != di->second.end(); si++) {
                            drmsg.params[3] = si->value;
                            drmsg.params[4] = intToBinSeq(*(si->timeleft));
                            handleRoutedMsg(drmsg);
                        }
                        
                        // then demote our data
                        indht.rdata[di->first] = di->second;
                        indht.data.erase(di);
                        direstart = true;
                    }
                }
                        
            } else {
                /* this person is farther - we've lost a node. Promote
                 * all of our redundant data into normal data */
                indht.data.insert(indht.rdata.begin(), indht.rdata.end());
                indht.rdata.clear();
                
                // update our immediate predecessor with all of our redundant data
                if (indht.neighbors[1] &&
                    dn_routes->find(*(indht.nbors_keys[1])) != dn_routes->end()) {
                    dhtNeighborUpdateData(indht, *((*dn_routes)[*(indht.nbors_keys[1])]),
                                          *(indht.neighbors[1]), 1);
                }
            }
        }
    }
}

void handleDHTDupMessage(conn_t *conn, Message msg)
{
    handleDHTMessage(conn, msg);
}

void handleDHTMessage(conn_t *conn, Message &msg)
{
    //msg.debug("DHT");
    
    if (CMD_IS("Had", 1, 1)) {
        REQ_PARAMS(6);
        
        if (!dhtForMe(msg, msg.params[1], encHashKey(msg.params[1] + msg.params[2]), NULL))
            return;
        
        // this is my data, store it
        // TODO: data ownership, redundancy, etc
        DHTInfo &indht = in_dhts[msg.params[1]];
        
        // make sure the name is legit
        if (msg.params[2].size() < 3) return;
        
        // make sure it's unique if it should be
        if (msg.params[2][0] & 0xF0 == 0) {
            // [0] & 0xF0 == 0 means it needs to be unique
            if (indht.data.find(msg.params[2]) != indht.data.end()) {
                // check if we don't already have this value
                dhtDataValue_t::iterator dvi;
                for (dvi = indht.data[msg.params[2]].begin();
                     dvi != indht.data[msg.params[2]].end();
                     dvi++) {
                    if (dvi->value == msg.params[3]) goto dataok;
                }
                /* the key is set but the value isn't there, so it's owned by
                 * somebody else */
                // FIXME: this should return SOMETHING to the sender
                return;
            }
        }
dataok:
        
        // get and set the data and timeout
        unsigned int tleft = (1 << (msg.params[2][0] & 0xF)) * 60;
        if (msg.params[4].size() == 4 &&
            (msg.params[2][0] & 0xF0) == 0) {
            unsigned int msgtleft =
                binSeqToInt(msg.params[4]);
            if (msgtleft < tleft)
                tleft = msgtleft;
        }
        indht.data[msg.params[2]].erase(DHTDataItem(msg.params[3], 0));
        indht.data[msg.params[2]].insert(DHTDataItem(msg.params[3], tleft));
        
        // now send the redundant info
        if (indht.neighbors[1]) {
            Route *toroute;
            if (*(indht.neighbors[1]) == pukeyhash) {
                toroute = NULL;
            } else if (dn_routes->find(*(indht.nbors_keys[1])) != dn_routes->end()) {
                toroute = (*dn_routes)[*(indht.nbors_keys[1])];
            } else {
                return;
            }
            
            // make the outgoing data
            Message rdmsg(1, "Hrd", 1, 1);
            if (toroute) {
                rdmsg.params.push_back(toroute->toBinSeq());
            } else {
                rdmsg.params.push_back("");
            }
            rdmsg.params.push_back(indht.HTI);
            rdmsg.params.push_back(msg.params[2]);
            rdmsg.params.push_back(msg.params[3]);
            rdmsg.params.push_back(intToBinSeq(tleft));
            rdmsg.params.push_back("");
            
            if (toroute) {
                handleRoutedMsg(rdmsg);
            } else {
                handleDHTDupMessage(conn, rdmsg);
            }
        }
        
        
    } else if (CMD_IS("Hag", 1, 1)) {
        REQ_PARAMS(8);
        
        if (!dhtForMe(msg, msg.params[1], encHashKey(msg.params[1] + msg.params[2]), &(msg.params[5])))
            return;
        
        // atomic add and get: divide it
        Message had(1, "Had", 1, 1);
        had.params.push_back(msg.params[0]);
        had.params.push_back(msg.params[1]);
        had.params.push_back(msg.params[2]);
        had.params.push_back(msg.params[3]);
        had.params.push_back(msg.params[4]);
        had.params.push_back(msg.params[7]);
        handleDHTDupMessage(conn, had);
        
        Message hga(1, "Hga", 1, 1);
        hga.params.push_back(msg.params[0]);
        hga.params.push_back(msg.params[1]);
        hga.params.push_back(msg.params[2]);
        hga.params.push_back(msg.params[5]);
        hga.params.push_back(msg.params[6]);
        handleDHTDupMessage(conn, hga);
        
    } else if (CMD_IS("Hga", 1, 1)) {
        REQ_PARAMS(5);
        
        if (!dhtForMe(msg, msg.params[1], encHashKey(msg.params[1] + msg.params[2]), &(msg.params[3])))
            return;
        
        DHTInfo &indht = in_dhts[msg.params[1]];
        
        // I should have this data, so return it
        Route retdata;
        
        if (indht.data.find(msg.params[2]) != indht.data.end()) {
            dhtDataValue_t &reqd = indht.data[msg.params[2]];
            dhtDataValue_t::iterator ri;
            for (ri = reqd.begin(); ri != reqd.end(); ri++) {
                retdata.push_back(ri->value);
            }
        }
        
        // build the return message
        Message ret(1, "Hra", 1, 1);
        
        Route rroute(msg.params[3]);
        if (rroute.size()) rroute.pop_back();
        rroute.reverse();
        rroute.push_back(msg.params[4]);
        ret.params.push_back(rroute.toBinSeq());
        
        ret.params.push_back(indht.HTI);
        ret.params.push_back(msg.params[2]);
        ret.params.push_back(retdata.toBinSeq());
        
        // handle or send it
        if (msg.params[4] != pukeyhash) {
            handleRoutedMsg(ret);
        } else {
            handleDHTDupMessage(conn, ret);
        }
        
    } else if (CMD_IS("Hgn", 1, 1)) {
        REQ_PARAMS(6);
        
        if (msg.params[5].size() < 1) return;
        
        if (in_dhts.find(msg.params[2]) == in_dhts.end()) return;
        DHTInfo &indht = in_dhts[msg.params[2]];
        
        int neighbor = 2;
        if (msg.params[5][0] == '\x00') neighbor = 1;
        
        /* check if this was actually the neighbor who was lost, in which case
         * we need to go one further */
        if ((indht.neighbors[neighbor] &&
             *(indht.nbors_keys[neighbor]) == msg.params[4]) ||
            !indht.neighbors[neighbor]) {
            // whoops, the other user doesn't want or need this
            if (neighbor == 2) neighbor = 3;
            else if (neighbor == 1) neighbor = 0;
        }
        
        /* FIXME: this should never happen */
        if (!indht.neighbors[neighbor]) {
            //printf("BAD\n");
            return;
        }
        
        // return an Hfr with the info they requested
        Message msgb(1, "Hfr", 1, 1);
        msgb.params.push_back(msg.params[3]);
        msgb.params.push_back(dn_name);
        msgb.params.push_back(indht.HTI);
        
        // construct the found-route
        Route froute(msg.params[3]);
        if (froute.size()) {
            froute.pop_back();
            froute.reverse();
            froute.push_back(pukeyhash);
        }
        if (*(indht.neighbors[neighbor]) == pukeyhash) {
            msgb.params.push_back(froute.toBinSeq());
            msgb.params.push_back(encExportKey());
        } else {
            if (dn_routes->find(*(indht.nbors_keys[neighbor])) != dn_routes->end()) {
                froute.append(*((*dn_routes)[*(indht.nbors_keys[neighbor])]));
            }
            msgb.params.push_back(froute.toBinSeq());
            msgb.params.push_back(*(indht.nbors_keys[neighbor]));
        }
        msgb.params.push_back(BinSeq("\x03\x00", 2));
        if (neighbor >= 2) msgb.params[5][0] = '\x06';
        
        // now send it
        if (msgb.params[3].size()) {
            handleRoutedMsg(msgb);
        } else {
            handleDHTDupMessage(conn, msgb);
        }
        
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
                
                BinSeq hashedKey = encHashKey(msg.params[5]);
                
                // get the return route
                Route toroute(msg.params[4]);
                // this route has us included, and doesn't have the end user
                Route rroute(toroute);
                if (rroute.size()) rroute.pop_back();
                rroute.reverse();
                rroute.push_back(hashedKey);
                BinSeq brroute = rroute.toBinSeq();
                
                // handle data changing hands
                dhtNeighborUpdateData(indht, rroute, hashedKey, 2);
                
                // add them to our DHT
                if (indht.nbors_keys[3]) delete indht.nbors_keys[3];
                indht.nbors_keys[3] = indht.nbors_keys[2];
                indht.nbors_keys[2] = new BinSeq(msg.params[5]);
                    
                if (indht.neighbors[3]) delete indht.neighbors[3];
                indht.neighbors[3] = indht.neighbors[2];
                indht.neighbors[2] = new BinSeq(encHashKey(msg.params[5]));
                
                
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
                    /*printf("NO ROUTE TO ");
                    outHex(neighbors[0].c_str(), neighbors[0].size());
                    printf("\n");*/
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
                        /*printf("NO ROUTE TO ");
                        outHex(neighbors[i].c_str(), neighbors[0].size());
                        printf("\n");*/
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
                        /*printf("NO ROUTE TO ");
                        outHex(indht.neighbors[3]->c_str(), indht.neighbors[3]->size());
                        printf("\n");*/
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
                        /*printf("NO ROUTE TO ");
                        outHex(indht.neighbors[1]->c_str(), indht.neighbors[1]->size());
                        printf("\n");*/
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
                        /*printf("NO ROUTE TO ");
                        outHex(indht.neighbors[3]->c_str(), indht.neighbors[3]->size());
                        printf("\n");*/
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
        Route rroute(msg.params[3]);
        if (msg.params[3].size() > 1) {
            dn_addRoute(msg.params[4], rroute);
        }
        
        if (msg.params[5][0] == '\x00') {
            // normal search
            Route *rt = new Route(msg.params[3]);
            recvFnd(rt, msg.params[1], msg.params[4]);
            delete rt;
            
            // could be a callback on this find
            if (dhtFnds.find(msg.params[1]) != dhtFnds.end()) {
                fndCallbackWData &fcwd = dhtFnds[msg.params[1]];
                if (fcwd.callback) {
                    fcwd.callback(msg.params[4], msg.params[1], fcwd.data);
                }
                dhtFnds.erase(msg.params[1]);
            }
            
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
            BinSeq hashedKey = encHashKey(msg.params[4]);
            Route rroute(msg.params[3]);
            
            // handle data changing hands
            dhtNeighborUpdateData(indht, rroute, hashedKey, nnum);
            
            // now actually set them in the DHT
            if (indht.nbors_keys[nnum])
                delete indht.nbors_keys[nnum];
            indht.nbors_keys[nnum] = new BinSeq(msg.params[4]);
                
            if (indht.neighbors[nnum])
                delete indht.neighbors[nnum];
            indht.neighbors[nnum] = new BinSeq(hashedKey);
            
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
                    /*printf("NO ROUTE TO ");
                    outHex(indht.neighbors[2]->c_str(), indht.neighbors[2]->size());
                    printf("\n");*/
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
        REQ_PARAMS(4);
        
        if (in_dhts.find(msg.params[1]) == in_dhts.end()) return;
        
        // Add this search result
        if (dhtSearches.find(msg.params[2]) != dhtSearches.end()) {
            // make the set from the route-style list
            Route vals(msg.params[3]);
            set<BinSeq> bsv;
            for (int i = 0; i < vals.size(); i++) {
                bsv.insert(vals[i]);
            }
            dhtSearches[msg.params[2]].searchResults[msg.params[1]] = bsv;
        }
        
        
    } else if (CMD_IS("Hrd", 1, 1)) {
        REQ_PARAMS(5);
        
        // store redundant info
        // FIXME: verification
        if (in_dhts.find(msg.params[1]) == in_dhts.end()) return;
        DHTInfo &indht = in_dhts[msg.params[1]];
        
        indht.rdata[msg.params[2]].erase(DHTDataItem(msg.params[3], 0));
        indht.rdata[msg.params[2]].insert(DHTDataItem(msg.params[3],
                                                      binSeqToInt(msg.params[4])));
        
    }
    
    map<BinSeq, DHTInfo>::iterator dhti;
    for (dhti = in_dhts.begin(); dhti != in_dhts.end(); dhti++) {
        dhtDebug(dhti->first);
    }
}

/* Convert an int to 4-byte big-endian */
BinSeq intToBinSeq(unsigned int val)
{
    BinSeq r("    ");
    r[0] = (val & 0xFF000000) >> 24;
    r[1] = (val & 0xFF0000  ) >> 16;
    r[2] = (val & 0xFF00    ) >> 8;
    r[3] =  val & 0xFF;
    return r;
}

/* Convert a binseq 4-byte big-endian to an int */
unsigned int binSeqToInt(const BinSeq &from)
{
    if (from.size() != 4) return 0;
    return
        (((unsigned int) from[0]) << 24) +
        (((unsigned int) from[1]) << 16) +
        (((unsigned int) from[2]) << 8) +
         ((unsigned int) from[3]);
}
