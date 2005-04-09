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

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "directnet.h"
#include "server.h"
#include "ui.h"

int server_sock;
#ifdef WIN32
BOOL sockets_init = FALSE;
#endif

void *serverAcceptLoop(void *ignore);

void sigterm_handler(int sig)
{
    pthread_exit(0);
}

#ifdef WIN32
void initSockets()
{
  WSADATA dw;
  if (sockets_init) return;
  if (WSAStartup(MAKEWORD(1,1), &dw) != 0) { /*check for version 1.1 */
	perror("WSAStartup - initializing winsock failed");
	exit(-1);
  }
  sockets_init = TRUE;
}
#endif

pthread_t *establishServer()
{
    int pthreadres;
    int yes=1;
    pthread_t *newthread;
    pthread_attr_t ptattr;
    
    struct sockaddr_in addr;
#ifdef WIN32
	initSockets();
#endif

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        exit(-1);
    }
    
    if (setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(-1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serv_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(addr.sin_zero), '\0', 8);
    
    if (bind(server_sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return 0;
    }
    
    if (listen(server_sock, 10) == -1) {
        perror("listen");
        exit(-1);
    }
    
    pthread_attr_init(&ptattr);
    newthread = (pthread_t *) malloc(sizeof(pthread_t));
    pthreadres = pthread_create(newthread, &ptattr, serverAcceptLoop, NULL);
    
    if (pthreadres == -1) {
        perror("pthread_create");
        exit(-1);
    }
    
    return newthread;
}

void *serverAcceptLoop(void *ignore)
{
    int sin_size, pthreadres, curfd;
    int *onfd_ptr, acceptfd;
    struct sockaddr_in rem_addr;
    pthread_attr_t ptattr;
    
    signal(SIGTERM, sigterm_handler);

    while(1) {
        sin_size = sizeof(struct sockaddr_in);
        
        acceptfd = accept(server_sock, (struct sockaddr *)&rem_addr, &sin_size);
        
        dn_lock(&dn_fd_lock);
        for (curfd = 0; curfd < DN_MAX_CONNS && fds[curfd]; curfd++);
	if (curfd == DN_MAX_CONNS) {
		/* no more room! */
		dn_unlock(&dn_fd_lock);
		continue;
	}
	
        fds[curfd] = acceptfd;
        if (fds[curfd] == -1) {
            fds[curfd] = 0;
            perror("accept");
            dn_unlock(&dn_fd_lock);
            continue;
        }
        if (curfd >= onfd) onfd = curfd + 1;
        dn_unlock(&dn_fd_lock);

        uiEstConn(inet_ntoa(rem_addr.sin_addr));
        
        onfd_ptr = malloc(sizeof(int));
        *onfd_ptr = curfd;
        //pids[onpid] = clone(communicator, client_stack, SIGCHLD | CLONE_FILES | CLONE_VM, (void *) onfd_ptr);
        pthread_attr_init(&ptattr);
	pthreads[onpthread] = (pthread_t *) malloc(sizeof(pthread_t));
        pthreadres = pthread_create(&pthreads[onpthread], &ptattr, communicator, (void *) onfd_ptr);
        
        onpthread++;
    }
    
    return NULL;
}
