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

#include "globals.h"
#include "hash.h"
#include "lock.h"

#ifdef GAIM_PLUGIN
int pluginMain(int argc, char **argv, char **envp);
#endif

extern pthread_t *serverPthread;
extern int serv_port;

extern int *fds, *pipe_fds, onfd, onpthread;
extern pthread_t **pthreads;
extern int onfd;
extern DN_LOCK dn_fd_lock;

extern DN_LOCK *pipe_locks;

extern DN_LOCK recFndHashLock; // Lock on the following hash
extern struct hashL *recFndLocks; // Locks on each fnd pthread (which wait for later fnds)
extern struct hashP *recFndPthreads; // And the pthreads themselves
extern struct hashS *weakRoutes; // List of weak routes

extern char dn_name[DN_NAME_LEN];
extern char dn_name_set;
extern struct hashI *dn_fds;

extern struct hashS *dn_routes;
extern struct hashS *dn_iRoutes;

extern struct hashI *dn_trans_keys;
extern int currentTransKey;
extern DN_LOCK dn_transKey_lock;

extern DN_LOCK displayLock;

extern char uiLoaded;

extern char dn_localip[24];

struct communInfo {
    int fdnum;
    int pthreadnum;
};

void newTransKey(char *into);

#endif // DN_DIRECTNET_H
