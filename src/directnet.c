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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <unistd.h>

#include "chat.h"
#include "config.h"
#include "directnet.h"
#include "dnconfig.h"
#include "globals.h"
#include "enc.h"
#include "hash.h"
#include "keepalive.h"
#include "lock.h"
#include "server.h"
#include "ui.h"
#include "whereami.h"

pthread_t *serverPthread, *keepalivePthread;
int serv_port = 3336;

int *fds, *pipe_fds, onpthread;
pthread_t **pthreads;
DN_LOCK pthread_lock;
int onfd;
DN_LOCK dn_fd_lock;

int pipes[DN_MAX_CONNS][2];
DN_LOCK *pipe_locks;

DN_LOCK recFndHashLock; // Lock on the following hash
struct hashL *recFndLocks; // Locks on each fnd pthread (which wait for later fnds)
struct hashP *recFndPthreads;
struct hashS *weakRoutes; // List of weak routes

char dn_name[DN_NAME_LEN+1];
char dn_name_set = 0;

char dn_leader[DN_NAME_LEN+1];
char dn_led = 0;
char dn_is_leader = 0;
DN_LOCK dn_leader_lock;

struct hashI *dn_fds;

struct hashS *dn_routes;
struct hashS *dn_iRoutes;

struct hashI *dn_trans_keys;
int currentTransKey;
DN_LOCK dn_transKey_lock;

char uiLoaded;
DN_LOCK displayLock; // Only one thread writing at a time.

char dn_localip[24];

char *homedir, *bindir;

char *findHome(char **envp);

#ifndef GAIM_PLUGIN
int main(int argc, char **argv, char **envp)
#else
int pluginMain(int argc, char **argv, char **envp)
#endif
{
    int i;
    char *binloc, *binnm, *cfgdir;
    pthread_t ac_pthrd;
    pthread_attr_t ac_pthrd_attr;
    
    serverPthread = NULL;
    keepalivePthread = NULL;
    
    // check our installed path
#ifndef GAIM_PLUGIN
    binloc = whereAmI(argv[0], &bindir, &binnm);
    if (binloc) {
        // we only need bindir
        free(binloc);
        free(binnm);
    } else {
        // make it up :(
        bindir = strdup("/usr/bin");
    }
#endif
    
    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[1], "-v", 2)) {
                printf("DirectNet version %s\n", VERSION);
                exit(0);
            } else if (!strncmp(argv[i], "-psn", 4)) {
                /* this is just here because OSX is weird */
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
    
    dn_lockInit(&dn_leader_lock);
    
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
    dn_lockInit(&pthread_lock);
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
    
    // We don't yet know our local IP
    dn_localip[0] = '\0';
    
    // Establish the server
    serverPthread = establishServer();
    
    // and the keepalive
    keepalivePthread = establishKeepalive();

    // Set home directory
    homedir = strdup(findHome(envp));
    
    // initialize configuration
    initConfig();
    
    // autoconnect in a thread
    if (pthread_attr_init(&ac_pthrd_attr) != 0) {
        perror("pthread_attr_init");
        exit(1);
    }
    if (pthread_create(&ac_pthrd, &ac_pthrd_attr, autoConnectThread, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    // Start the UI
    uiInit(argc, argv, envp);
    
    // make sure autoconnect is done
    pthread_join(ac_pthrd, NULL);
    
#ifndef GAIM_PLUGIN /* The Gaim plugin can't do this cleanly (yet) */
#ifndef __WIN32 /* win32-pthreads hangs in here, but exits cleanly on a normal exit */
    
    // When the UI has exited, we're done.
    if (serverPthread) {
        pthread_cancel(*serverPthread);
        pthread_join(*serverPthread, NULL);
    }
    
    if (keepalivePthread) {
        pthread_cancel(*keepalivePthread);
        pthread_join(*keepalivePthread, NULL);
    }
    
    dn_lock(&pthread_lock);
    for (i = 0; i < onpthread; i++) {
        if (pthreads[i]) {
            pthread_cancel(*(pthreads[i]));
            pthread_join(*(pthreads[i]), NULL);
        }
    }
    dn_unlock(&pthread_lock);
    
    free(fds);
    free(pipe_fds);
    free(pipe_locks);
    free(pthreads);
    //free(fnd_pthreads);
    
    pthread_exit(NULL);
    
#endif
#endif
    
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
   
#ifdef WIN32
    // On Windoze we'll accept curdir
    {
        char *cdir = (char *) malloc(1024);
        if (!cdir) { perror("malloc"); exit(1); }
        if (getcwd(cdir, 1024))
            return cdir;
        // if no curdir, just use bindir
        return bindir;
    }
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
