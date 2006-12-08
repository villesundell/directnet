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

#ifndef DN_DHT_H
#define DN_DHT_H

#include <map>
#include <set>
#include <string>
#include <vector>
using namespace std;

#include "connection.h"
#include "route.h"

typedef map<BinSeq, set<BinSeq> > binseqHash;

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
    binseqHash data;
    
    // the data stored redundantly for the DHT
    binseqHash rdata;
};

// map the DHTs we're members of to DHTInfo nodes
extern map<BinSeq, DHTInfo> in_dhts;

/* list of DHTs we're currently in
 * must: set to true if we need to be in a DHT, and should create one if we're
 *       not */
Route dhtIn(bool must);

/* join a DHT (but don't establish yet)
 * ident: the identifier of the DHT
 * rep: our initial representative (for first requests) */
void dhtJoin(const BinSeq &ident, const BinSeq &rep);

/* create a DHT and establish ourself into it */
void dhtCreate();

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

/* Send a properly-formed message over the DHT
 * params same as dhtForMe */
void dhtSendMsg(Message &msg, const BinSeq &ident, const BinSeq &key, BinSeq *route);

/* Send a properly-formatted message over all DHTs
 * params same as dhtForMe, but ident is a pointer, the value will change */
void dhtAllSendMsg(Message &msg, BinSeq *ident, const BinSeq &key, BinSeq *route);

/* Same as handleDHTMessage below, but duplicate the msg object */
void handleDHTDupMessage(conn_t *conn, Message msg);

/* Handle a messgae related to DHTs
 * conn: the connection
 * msg: the message itself */
void handleDHTMessage(conn_t *conn, Message &msg);

#endif
