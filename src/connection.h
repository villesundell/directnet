/*
 * Copyright 2004, 2005, 2006, 2007  Gregor Richards
 * Copyright 2006  Bryan Donlan
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

#ifndef DN_CONNECTION_H
#define DN_CONNECTION_H

#include <map>
#include <set>
#include <string>
using namespace std;

#include "dn_event.h"
#include "message.h"
#include "route.h"

enum cev_state {
    CDN_EV_IDLE, CDN_EV_EVENT, CDN_EV_DYING
};

class connection {
    public:
    ~connection();
        
    connection *prev, *next;
    int fd;
    enum cev_state state;
        
    unsigned char *inbuf, *outbuf;
    size_t inbuf_sz, outbuf_sz;
    size_t inbuf_p, outbuf_p;
        
    BinSeq *enckey;
    bool outgoing;
    string *outgh;
    int outgp;
    
    bool requested; // if the user requested this connection
        
    dn_event_fd fd_ev;
    dn_event_timer ping_ev;
    
    std::set<connection *>::iterator active_it;
};
 
typedef connection conn_t;

/* Establish a connection (for use by the UI)
 * to: user, hostname or IP to connect to
 */
void establishConnection(const string &to);

/* Begin peer communication on an opened file handle
 * If the connection is outgoing, include outgh and outgp
 */
conn_t *init_comms(int fd, const string *outgh, int outgp);

/* Send a message (for use by the UI)
 * to: user to send to
 * msg: the message
 * returns: 1 on success, 0 otherwise */
int sendMsg(const string &to, const string &msg);

/* Send your authentication key (for use by the UI)
 * to: user to send to
 * returns: 1 on success, 0 otherwise */
int sendAuthKey(const string &to);

/* Set away status (for use by the UI)
 * msg: the away message or NULL for back */
void setAway(const string *msg);

/* Get away status and message
 * Return NULL if not away, message otherwise */
const string *getAway();

/* Send a command buffer to an fd
 * fdnum: the fd to send to
 * buf: the buffer to send */
void sendCmd(struct connection *conn, Message &msg);

/* Continue a routed message or tell the calling function to handle it
 * command: the command
 * vera, verb: the major and minor version of the command
 * params: the parameters
 * returns: true if the calling function needs to handle it */
bool handleRoutedMsg(const Message &msg);

/* Send an unrouted message
 * fromfd: the fd NOT to send it to, -1 to send everywhere
 * outbuf: the buffer to send */
void emitUnroutedMsg(conn_t *from, Message &omsg);

/* Receive a find of some sort (so add a user)
 * route: The route to the distant user
 * name: The name of the distant user
 * key: The encryption key of the distant user */
void recvFnd(Route *route, const BinSeq &name, const BinSeq &key);

/* The main communication method for a connection 
 * fdnum_voidptr: a void * to an int with the fdnum */
void *communicator(void *fdnum_voidptr);

/* A node has disconnected.  Do we care? */
void disNode(const BinSeq &key);

/* Am I online (do I have any connections)? */
bool dnOnline();

// convenience macros for handle*Msg
#define REQ_PARAMS(x) if (msg.params.size() < x) return
#define CMD_IS(x, y, z) (msg.cmd == x && msg.ver[0] == y && msg.ver[1] == z)

#endif // DN_CONNECTION_H
