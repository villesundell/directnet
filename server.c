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

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "directnet.h"
#include "server.h"

#define DN_PORT 3336

int server_sock;

void *serverAcceptLoop(void *ignore);

void sigterm_handler(int sig)
{
    exit(0);
}

pthread_t establishServer()
{
    int pthreadres;
    int yes=1;
    pthread_t newthread;
    pthread_attr_t ptattr;
    
    struct sockaddr_in addr;
    
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
    addr.sin_port = htons(DN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(addr.sin_zero), '\0', 8);
    
    if (bind(server_sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    
    if (listen(server_sock, 10) == -1) {
        perror("listen");
        exit(-1);
    }
    
    pthread_attr_init(&ptattr);
    pthreadres = pthread_create(&newthread, &ptattr, serverAcceptLoop, NULL);
    
    if (pthreadres == -1) {
        perror("pthread_create");
        exit(-1);
    }
    
    return newthread;
}

void *serverAcceptLoop(void *ignore)
{
    int sin_size, pthreadres, curfd;
    int *onfd_ptr;
    struct sockaddr_in rem_addr;
    pthread_attr_t ptattr;
    
    signal(SIGTERM, sigterm_handler);

    while(1) {
        sin_size = sizeof(struct sockaddr_in);
        curfd = onfd;
        onfd++;
        fds[curfd] = accept(server_sock, (struct sockaddr *)&rem_addr, &sin_size);
        if (fds[curfd] == -1) {
            perror("accept");
            continue;
        }

        printf("server: got connection from %s\n",
               inet_ntoa(rem_addr.sin_addr));
        
        onfd_ptr = malloc(sizeof(int));
        *onfd_ptr = curfd;
        //pids[onpid] = clone(communicator, client_stack, SIGCHLD | CLONE_FILES | CLONE_VM, (void *) onfd_ptr);
        pthread_attr_init(&ptattr);
        pthreadres = pthread_create(&pthreads[onpthread], &ptattr, communicator, (void *) onfd_ptr);
        
        onpthread++;
    }
    
    return NULL;
}
