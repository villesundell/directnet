/*
 * Copyright 2004, 2005 Gregor Richards
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

void establishConnection(char *to);
int sendMsg(char *to, char *msg);
void sendFnd(char *to);
void joinChat(char *chat);
void leaveChat(char *chat);
void sendChat(char *to, char *msg);
void setAway(char *msg);

void buildCmd(char *into, char *command, char vera, char verb, char *param);
void addParam(char *into, char *newparam);
void sendCmd(int fdnum, char *buf);
int handleRoutedMsg(char *command, char vera, char verb, char **params);
void emitUnroutedMsg(int fromfd, char *outbuf);

void *communicator(void *fdnum_voidptr);

#endif // DN_CONNECTION_H
