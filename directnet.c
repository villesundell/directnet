/*
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
#include "server.h"
#include "ui.h"

int *fds, onfd, onpthread;
pthread_t *pthreads;
char dn_name[1024];
struct hashKey **dn_fds;
struct hashKeyS **dn_routes;
struct hashKey **dn_trans_keys;

/*void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}*/

char *findHome(char **envp);

int main(int argc, char **argv, char **envp)
{
    pthread_t serverPthread;
    int i;
    
    fds = (int *) malloc(1024 * sizeof(int));
    onfd = 0;
    pthreads = (pthread_t *) malloc(1024 * sizeof(pthread_t));
    onpthread = 0;
    
    dn_fds = hashCreate(1024);
    dn_routes = hashSCreate(1024);
    dn_trans_keys = hashCreate(65536);
    currentTransKey = 0;
    
    serverPthread = establishServer();

    sprintf(gpghomedir, "%s/.directnet", findHome(envp));

    uiInit(envp);
    
    pthread_kill(serverPthread, SIGTERM);
    
    for (i = 0; i < onpthread; i++) {
        //kill(pids[i], SIGTERM);
        pthread_kill(pthreads[i], SIGTERM);
        //waitpid(pids[i], NULL, 0);
    }
    
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
