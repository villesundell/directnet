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

#ifndef DN_DHT_H
#define DN_DHT_H

#include <map>
#include <set>
#include <string>
#include <vector>
using namespace std;

#include "connection.h"
#include "route.h"

// A single item of data in the DHT
class DHTDataItem {
    public:
    inline DHTDataItem(const DHTDataItem &copy) {
        value = copy.value;
        timeleft = new unsigned int;
        *timeleft = *(copy.timeleft);
    }
    inline DHTDataItem(const BinSeq &sv, unsigned int stl) {
        value = sv;
        timeleft = new unsigned int;
        *timeleft = stl;
    }
    inline ~DHTDataItem() {
        if (timeleft) delete timeleft;
    }
    inline bool operator==(const DHTDataItem &v) const {
        return (value == v.value);
    }
    inline bool operator<(const DHTDataItem &v) const {
        return (value < v.value);
    }
    inline bool operator>(const DHTDataItem &v) const {
        return (value > v.value);
    }
    BinSeq value;
    unsigned int *timeleft;
};

typedef set<DHTDataItem> dhtDataValue_t;
typedef map<BinSeq, dhtDataValue_t > dhtData_t;

// DHT info is stored here
class DHTInfo {
    public:
    DHTInfo();
    BinSeq HTI; // hash table identifier
    BinSeq *rep; // our representative
    BinSeq *neighbors[4]; // -2, -1, 1, 2
    BinSeq *nbors_keys[4];
    vector<BinSeq *> real_divisions;
    vector<BinSeq *> real_divs_keys;
    vector<BinSeq *> best_divisions;
    bool established;
    bool estingNeighbors; // FIXME: should be timed
    
    // the data stored for the DHT
    dhtData_t data;
    
    // the data stored redundantly for the DHT
    dhtData_t rdata;
};

// map the DHTs we're members of to DHTInfo nodes
extern map<BinSeq, DHTInfo> in_dhts;

// searches currently in progress
typedef map<BinSeq, set<BinSeq> > searchResults_t;
// callback form: key, values, data
typedef void (*dhtSearchCallback)(const BinSeq &, const set<BinSeq> &, void *);
class DHTSearchInfo {
    public:
    searchResults_t searchResults;
    dhtSearchCallback callback;
    void *data;
};
extern map<BinSeq, DHTSearchInfo> dhtSearches;

// finds currently in progress (finds are for users, searches are for data)
// callback form: enc key, name, data
typedef void (*fndCallback)(const BinSeq &, const BinSeq &, void *);
class fndCallbackWData {
    public:
    fndCallback callback;
    void *data;
    
    inline void set(fndCallback sc, void *sd)
    {
        callback = sc;
        data = sd;
    }
};
extern map<BinSeq, fndCallbackWData> dhtFnds;

/* list of DHTs we're currently in
 * must: set to true if we need to be in a DHT, and should create one if we're
 *       not */
Route dhtIn(bool must);

/* join a DHT (but don't establish yet)
 * ident: the identifier of the DHT
 * rep: our initial representative (for first requests) */
void dhtJoin(const BinSeq &ident, const BinSeq &rep);

/* create a DHT and establish ourself into it */
void dhtCreate(const BinSeq &key);

/* are we fully established into this DHT?
 * ident: The identification for the DHT
 * returns: true if fully established */
bool dhtEstablished(const BinSeq &ident);

enum {  DHT_ESTABLISH_NEIGHBORS,
        DHT_ESTABLISH_DIVISIONS };

/* do a step to establish ourself
 * ident: into what DHT? */
void dhtEstablish(const BinSeq &ident, int step = -1);

/* do all of our DHT links have correspondant routes?  If not, reestablish */
void dhtCheck();

/* what's the next hop for a search?
 * ident: what DHT?
 * key: the key being searched for
 * closer: rather than using >=, decide hops by closeness
 * return: the encryption key of the next hop, "" for this node */
BinSeq dhtNextHop(const BinSeq &ident, const BinSeq &key, bool closer = false);

/* is this message for us?  If not, continue it
 * msg: the message itself
 * ident: what DHT?
 * key: the key being searched for
 * route: where to append the route
 * closer: rather than using >=, decide hops by closeness
 * return: true if for us */
bool dhtForMe(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route, bool closer = false);

/* Send a search for data, with a callback for when it comes */
void dhtSendSearch(const BinSeq &key, dhtSearchCallback callback, void *data);

/* Send a properly-formed add message over the DHT, with a refresher loop */
void dhtSendAdd(const BinSeq &key, const BinSeq &value, DHTInfo *dht = NULL);

/* Send an atomic add/search */
void dhtSendAddSearch(const BinSeq &key, const BinSeq &value,
                      dhtSearchCallback callback, void *data,
                      DHTInfo *dht = NULL);

/* Send a properly-formed search message over the DHT
 * params same as dhtForMe */
void dhtSendMsg(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route);

/* Send a properly-formatted message over all DHTs
 * params same as dhtForMe, but ident is a pointer, the value will change */
void dhtAllSendMsg(Message &msg, BinSeq *ident, const BinSeq &key, BinSeq *route);

/* Send a find by username
 * to: user to search for
 * callback: function to call (or NULL) upon successful find
 * data: data to send to the callback
 * NOTE: this does not have the dht prefix due to history */
void sendFnd(const string &to, fndCallback callback, void *data);

/* Send a find by username (for use by the UI)
 * to: user to search for */
inline void sendFnd(const string &to) { sendFnd(to, NULL, NULL); }

/* Send a find by key
 * key: key to search for
 * callback: function to call (or NULL) upon successful find
 * data: data to send to the callback */
void dhtFindKey(const BinSeq &key, fndCallback callback, void *data);

/* Same as handleDHTMessage below, but duplicate the msg object */
void handleDHTDupMessage(conn_t *conn, Message msg);

/* Handle a messgae related to DHTs
 * conn: the connection
 * msg: the message itself */
void handleDHTMessage(conn_t *conn, Message &msg);

/* Convert an int to 4-byte big-endian */
BinSeq intToBinSeq(unsigned int val);

/* Convert a binseq 4-byte big-endian to an int */
unsigned int binSeqToInt(const BinSeq &from);

#endif
