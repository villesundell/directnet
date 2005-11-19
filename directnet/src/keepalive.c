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

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef WIN32
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#include "directnet.h"
#include "globals.h"
#include "keepalive.h"

void *keepaliveLoop(void *ignore);

void k_sigterm_handler(int sig)
{
    pthread_exit(0);
}

pthread_t *establishKeepalive()
{
    pthread_t *newthread;
    pthread_attr_t ptattr;
    int pthreadres;
    
    newthread = (pthread_t *) malloc(sizeof(pthread_t));
    if (newthread == NULL) {
        perror("malloc");
        exit(-1);
    }
    
    pthread_attr_init(&ptattr);
    pthreadres = pthread_create(newthread, &ptattr, keepaliveLoop, NULL);
    
    if (pthreadres == -1) {
        perror("pthread_create");
        exit(-1);
    }
    
    return newthread;
}

void *keepaliveLoop(void *ignore)
{
    int i;
    
    signal(SIGTERM, k_sigterm_handler);
    
    /* every five minutes, send a ping to all fds */
    while (1) {
        sleep(DN_KEEPALIVE_TIMER);
        
        for (i = 0; i < DN_MAX_CONNS; i++) {
            if (fds[i]) {
                /* the packet is raw coded in */
                send(fds[i], "pin\x01\x01;\0", 8, 0);
            }
        }
    }
}
