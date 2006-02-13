/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
extern pthread_t *keepalivePthread;
extern int serv_port;

extern int *fds, *pipe_fds, onfd, onpthread;
extern pthread_t **pthreads;
extern DN_LOCK pthread_lock;
extern int onfd;
extern DN_LOCK dn_fd_lock;

extern DN_LOCK *pipe_locks;

extern DN_LOCK recFndHashLock; // Lock on the following hash
extern struct hashL *recFndLocks; // Locks on each fnd pthread (which wait for later fnds)
extern struct hashP *recFndPthreads; // And the pthreads themselves
extern struct hashS *weakRoutes; // List of weak routes

// our name
extern char dn_name[DN_NAME_LEN+1];
extern char dn_name_set; // this is really a bool

// our leader
extern char dn_leader[DN_NAME_LEN+1];
extern char dn_led; // really a bool
extern DN_LOCK dn_leader_lock;

// file descriptors by name
extern struct hashI *dn_fds;

// route by name
extern struct hashS *dn_routes;

// intermediate routes by name
extern struct hashS *dn_iRoutes;

// has a key been used yet?
extern struct hashI *dn_trans_keys;

// our position in the transkey list
extern int currentTransKey;

// lock on transkey stuff
extern DN_LOCK dn_transKey_lock;

// lock on display
extern DN_LOCK displayLock;

// is the ui loaded?
extern char uiLoaded;

// our IP
extern char dn_localip[24];

// stuff for relocatability
extern char *homedir, *bindir;

struct communInfo {
    int fdnum;
    int pthreadnum;
};

void newTransKey(char *into);

#endif // DN_DIRECTNET_H
