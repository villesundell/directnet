/*
 * Copyright 2004 Gregor Richards
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

#ifndef DN_DIRECTNET_H
#define DN_DIRECTNET_H

#include <pthread.h>

#include "hash.h"
#include "lock.h"

extern int *fds, *pipe_fds, onfd, onpthread;
extern pthread_t *pthreads;

extern DN_LOCK *pipe_locks;

extern pthread_t *fnd_pthreads;
extern char *weakRoutes[1024];

extern char dn_name[1024];
extern struct hashKey **dn_fds;

extern struct hashKeyS **dn_routes;
extern char *dn_route_by_num[1024];

extern struct hashKey **dn_trans_keys;
extern int currentTransKey;

extern char uiLoaded;

void newTransKey(char *into);

#endif // DN_DIRECTNET_H
