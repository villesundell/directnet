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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <unistd.h>

#include "chat.h"
#include "directnet.h"
#include "globals.h"
#include "gpg.h"
#include "hash.h"
#include "lock.h"
#include "server.h"
#include "ui.h"

pthread_t *serverPthread;
int serv_port = 3336;

int *fds, *pipe_fds, onpthread;
pthread_t **pthreads;
int onfd;
DN_LOCK dn_fd_lock;

int pipes[DN_MAX_CONNS][2];
DN_LOCK *pipe_locks;

DN_LOCK recFndHashLock; // Lock on the following hash
struct hashL *recFndLocks; // Locks on each fnd pthread (which wait for later fnds)
struct hashP *recFndPthreads;
struct hashS *weakRoutes; // List of weak routes

char dn_name[DN_NAME_LEN];
char dn_name_set = 0;
struct hashI *dn_fds;

struct hashS *dn_routes;
struct hashS *dn_iRoutes;

struct hashI *dn_trans_keys;
int currentTransKey;
DN_LOCK dn_transKey_lock;

char uiLoaded;
DN_LOCK displayLock; // Only one thread writing at a time.

char *findHome(char **envp);

#ifndef GAIM_PLUGIN
int main(int argc, char **argv, char **envp)
#else
int pluginMain(int argc, char **argv, char **envp)
#endif
{
    serverPthread = NULL;
    int i;
    
    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[1], "-v", 2)) {
                printf("DirectNet version ALPHA 0.4\n");
                exit(0);
            } else if (!strncmp(argv[i], "-p", 2)) {
                i++;
                if (!argv[i]) {
                    fprintf(stderr, "-p must have an argument\n");
                    exit(1);
                }
                serv_port = atoi(argv[i]);
            } else {
                fprintf(stderr, "Use:\n%s [-v] [-p port]\n", argv[0]);
                fprintf(stderr, "  -v: Display the version number and quit.\n");
                fprintf(stderr, "  -p: Set the port to listen for connections on (default 3336).\n\n");
                exit(1);
            }
        }
            
    }
    
    // fds is an array containing every file descriptor.  fdnums are indexes into this array
    fds = (int *) malloc(DN_MAX_CONNS * sizeof(int));
    memset(fds, 0, DN_MAX_CONNS * sizeof(int));
    onfd = 0;
    // dn_fd_lock is a lock over fds[]
    dn_lockInit(&dn_fd_lock);
    // pipe_fds is an equivilant array for the output pipes that throttle bandwidth
    pipe_fds = (int *) malloc(DN_MAX_CONNS * sizeof(int));
    
    pipe_locks = (DN_LOCK *) malloc(DN_MAX_CONNS * sizeof(DN_LOCK));
    memset(pipe_locks, 0, DN_MAX_CONNS * sizeof(DN_LOCK));
    
    // pthreads is an array of every active connection's pthread_t
    pthreads = (pthread_t **) malloc(DN_MAX_CONNS * sizeof(pthread_t *));
    memset(pthreads, 0, DN_MAX_CONNS * sizeof(pthread_t *));
    onpthread = 0;
    
    // these are described above
    dn_lockInit(&recFndHashLock);
    recFndLocks = hashLCreate();
    recFndPthreads = hashPCreate();
    
    /* Upon receiving a fnd, the node then continues to accept alternate fnds.  Every time it
       receives one, it goes through the current route and new route and only keeps the ones that
       appear in both.  That way, it ends up with a list of nodes that have no backup.  If there
       are any such nodes, it needs to attempt a direct connection to strengthen the network. */
    weakRoutes = hashSCreate();
    
    // This stores fd numbers by name
    dn_fds = hashICreate();
    
    // This stores routes by name
    dn_routes = hashSCreate();
    // This stores intermediate routes, for response on broken routes
    dn_iRoutes = hashSCreate();
    
    // This hash stores the state of all nonrepeating unrouted messages
    dn_trans_keys = hashICreate();
    currentTransKey = 0;
    dn_lockInit(&dn_transKey_lock);
    
    // This hash stores whether we're in certain chats
    dn_chats = hashSCreate();
    // And this locks that hash
    dn_lockInit(&dn_chat_lock);
    
    /* Set uiLoaded to 0 - this is merely a convenience for UIs that need to monitor whether
       they're loaded yet */
    uiLoaded = 0;
    
    // Initialize the lock for output
    dn_lockInit(&displayLock);
    
    // Establish the server
    serverPthread = establishServer();

    // Set GPG's home directory
    sprintf(gpghomedir, "%.245s/.directnet", findHome(envp));

    // Start the UI
    uiInit(argc, argv, envp);

#ifndef GAIM_PLUGIN
    
    // When the UI has exited, we're done.
    serverPthread ? pthread_kill(*serverPthread, SIGTERM) : 0;
    
    for (i = 0; i < onpthread; i++) {
        //kill(pids[i], SIGTERM);
        pthreads[i] ? pthread_kill(*(pthreads[i]), SIGTERM) : 0;
        //waitpid(pids[i], NULL, 0);
    }
    
    free(fds);
    free(pipe_fds);
    free(pipe_locks);
    free(pthreads);
    //free(fnd_pthreads);
    
    pthread_exit(NULL);
    
    return 0;
    
#endif
}

char *findHome(char **envp)
{
    int i;
    
    for (i = 0; envp[i] != NULL; i++) {
        if (!strncmp(envp[i], "HOME=", 5)) {
            return envp[i]+5;
        }
    }
   
#ifdef WIN32
    // On Windoze we'll accept something invalid
    return "c:/";
#endif
    
    fprintf(stderr, "Couldn't find your HOME directory.\n");
    exit(-1);
}

void newTransKey(char *into)
{
    dn_lock(&dn_transKey_lock);
    sprintf(into, "%s%d", dn_name, currentTransKey);
    hashISet(dn_trans_keys, into, 1);
    currentTransKey++;
    dn_unlock(&dn_transKey_lock);
}
