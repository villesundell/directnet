/*
 * Copyright 2004, 2005  Gregor Richards
 * Copyright 2006 Bryan Donlan
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

#ifndef DN_CONNECTION_H
#define DN_CONNECTION_H

#include <string>
using namespace std;

#include "message.h"

struct connection;
typedef struct connection conn_t;

/* Establish a connection (for use by the UI)
 * to: user, hostname or IP to connect to
 */
void establishConnection(const string &to);

/* Begin peer communication on an opened file handle
 * If the connection is outgoing, include outgh and outgp
 */
void init_comms(int fd, const string *outgh, int outgp);

/* Send a message (for use by the UI)
 * to: user to send to
 * msg: the message
 * returns: 1 on success, 0 otherwise */
int sendMsg(const string &to, const string &msg);

/* Send your authentication key (for use by the UI)
 * to: user to send to
 * returns: 1 on success, 0 otherwise */
int sendAuthKey(const string &to);

/* Send a find (for use by the UI)
 * to: user to search for */
void sendFnd(const string &to);

/* Join a chat (for use by the UI)
 * chat: room to join */
void joinChat(const string &chat);

/* Leave a chat (for use by the UI)
 * chat: room to leave */
void leaveChat(const string &chat);

/* Send a message on a chat (for use by the UI)
 * to: room to send to
 * msg: the message */
void sendChat(const string &to, const string &msg);

/* Set away status (for use by the UI)
 * msg: the away message or NULL for back */
void setAway(const string *msg);

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

/* The main communication method for a connection 
 * fdnum_voidptr: a void * to an int with the fdnum */
void *communicator(void *fdnum_voidptr);

#endif // DN_CONNECTION_H
