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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "directnet.h"
#include "gpg.h"
#include "hash.h"
#include "lock.h"
#include "server.h"
#include "ui.h"

int *fds, *pipe_fds, onfd, onpthread;
pthread_t *pthreads;

int pipes[1024][2];
DN_LOCK *pipe_locks;

/*pthread_t *fnd_pthreads;
char *weakRoutes[1024];*/
DN_LOCK recFndHashLock; // Lock on the following hash
struct hashKeyL **recFndLocks; // Locks on each fnd pthread (which wait for later fnds)
struct hashKeyP **recFndPthreads;
struct hashKeyS **weakRoutes; // List of weak routes

char dn_name[1024];
struct hashKey **dn_fds;

struct hashKeyS **dn_routes;
struct hashKeyS **dn_iRoutes;

struct hashKey **dn_trans_keys;
int currentTransKey;

char uiLoaded;

char *findHome(char **envp);

int main(int argc, char **argv, char **envp)
{
    pthread_t serverPthread;
    int i;
    
    if (argc >= 2) {
        if (!strncmp(argv[1], "-v", 2)) {
            printf("DirectNet version ALPHA 0.2\n");
            exit(0);
        }
    }
    
    // fds is an array containing every file descriptor.  fdnums are indexes into this array
    fds = (int *) malloc(1024 * sizeof(int));
    // pipe_fds is an equivilant array for the output pipes that throttle bandwidth
    pipe_fds = (int *) malloc(1024 * sizeof(int));
    onfd = 0;
    
    pipe_locks = (DN_LOCK *) malloc(1024 * sizeof(DN_LOCK));
    memset(pipe_locks, 0, 1024 * sizeof(DN_LOCK));
    
    // pthreads is an array of every active connection's pthread_t
    pthreads = (pthread_t *) malloc(1024 * sizeof(pthread_t));
    onpthread = 0;
    
    /* fnd_pthreads is an array of the temporary pthreads made to collect extra routes after
       receiving an initial fnd * /
    fnd_pthreads = (pthread_t *) malloc(1024 * sizeof(pthread_t));
    memset(fnd_pthreads, 0, 1024 * sizeof(pthread_t));*/
    
    // these are described above
    dn_lockInit(&recFndHashLock);
    recFndLocks = hashLCreate(1024);
    recFndPthreads = hashPCreate(1024);
    
    /* Upon receiving a fnd, the node then continues to accept alternate fnds.  Every time it
       receives one, it goes through the current route and new route and only keeps the ones that
       appear in both.  That way, it ends up with a list of nodes that have no backup.  If there
       are any such nodes, it needs to attempt a direct connection to strengthen the network. */
    weakRoutes = hashSCreate(1024);
    //memset(weakRoutes, 0, 1024 * sizeof(char *));
    
    // This stores fd numbers by name
    dn_fds = hashCreate(1024);
    
    // This stores routes by name
    dn_routes = hashSCreate(1024);
    // This stores intermediate routes, for response on broken routes
    dn_iRoutes = hashSCreate(1024);
    
    // This hash stores the state of all nonrepeating unrouted messages
    dn_trans_keys = hashCreate(65536);
    currentTransKey = 0;
    
    /* Set uiLoaded to 0 - this is merely a convenience for UIs that need to monitor whether
       they're loaded yet */
    uiLoaded = 0;
    
    // Establish the server
    serverPthread = establishServer();

    // Set GPG's home directory
    sprintf(gpghomedir, "%.245s/.directnet", findHome(envp));

    // Start the UI
    uiInit(argc, argv, envp);
    
    // When the UI has exited, we're done.
    serverPthread ? pthread_kill(serverPthread, SIGTERM) : 0;
    
    for (i = 0; i < onpthread; i++) {
        //kill(pids[i], SIGTERM);
        pthreads[i] ? pthread_kill(pthreads[i], SIGTERM) : 0;
        //waitpid(pids[i], NULL, 0);
    }
    
    free(fds);
    free(pipe_fds);
    free(pipe_locks);
    free(pthreads);
    //free(fnd_pthreads);
    
    pthread_exit(NULL);
    
    return 0;
}

char *findHome(char **envp)
{
    int i;
    
    for (i = 0; envp[i] != NULL; i++) {
        if (!strncmp(envp[i], "HOME=", 5)) {
            return envp[i]+5;
        }
    }
    
    fprintf(stderr, "Couldn't find your HOME directory.\n");
    exit(-1);
}

void newTransKey(char *into)
{
    sprintf(into, "%s%d", dn_name, currentTransKey);
    hashSet(dn_trans_keys, into, 1);
}
