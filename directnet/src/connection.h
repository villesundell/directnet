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
 */

#ifndef DN_CONNECTION_H
#define DN_CONNECTION_H

/* Establish a connection (for use by the UI)
 * to: user, hostname or IP to connect to
 * returns: 1 on success, 0 otherwise */
int establishConnection(char *to);

/* Send a message (for use by the UI)
 * to: user to send to
 * msg: the message
 * returns: 1 on success, 0 otherwise */
int sendMsg(char *to, char *msg);

/* Send your authentication key (for use by the UI)
 * to: user to send to
 * returns: 1 on success, 0 otherwise */
int sendAuthKey(char *to);

/* Send a find (for use by the UI)
 * to: user to search for */
void sendFnd(char *to);

/* Join a chat (for use by the UI)
 * chat: room to join */
void joinChat(char *chat);

/* Leave a chat (for use by the UI)
 * chat: room to leave */
void leaveChat(char *chat);

/* Send a message on a chat (for use by the UI)
 * to: room to send to
 * msg: the message */
void sendChat(char *to, char *msg);

/* Set away status (for use by the UI)
 * msg: the away message */
void setAway(const char *msg);

/* Build the top of a command buffer
 * into: buffer to fill
 * command: command to use
 * vera: version major part
 * verb: version minor part
 * param: first parameter */
void buildCmd(char *into, char *command, char vera, char verb, char *param);

/* Add a parameter to a command buffer
 * into: buffer to add to
 * newparam: the param to add */
void addParam(char *into, char *newparam);

/* Send a command buffer to an fd
 * fdnum: the fd to send to
 * buf: the buffer to send */
void sendCmd(int fdnum, char *buf);

/* Continue a routed message or tell the calling function to handle it
 * command: the command
 * vera, verb: the major and minor version of the command
 * params: the parameters
 * returns: 1 if the calling function needs to handle it */
int handleRoutedMsg(char *command, char vera, char verb, char **params);

/* Send an unrouted message
 * fromfd: the fd NOT to send it to, -1 to send everywhere
 * outbuf: the buffer to send */
void emitUnroutedMsg(int fromfd, char *outbuf);

/* The main communication method for a connection (a pthread)
 * fdnum_voidptr: a void * to an int with the fdnum */
void *communicator(void *fdnum_voidptr);

#endif // DN_CONNECTION_H
