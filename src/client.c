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

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "connection.h"
#include "directnet.h"

void establishClient(char *destination)
{
    int pthreadres, curfd;
    int *onfd_ptr;
    struct hostent *he;
    struct sockaddr_in addr;
    //struct in_addr inIP;
    pthread_attr_t ptattr;
    char *hostname;
    int i, port;

    /* split 'destination' into 'hostname' and 'port' */
    hostname = strdup(destination);
    for (i = 0; hostname[i] != ':' && hostname[i] != '\0'; i++);
    if (hostname[i] == ':') {
        /* it has a port */
        hostname[i] = '\0';
        port = atoi(hostname + i + 1);
    } else {
        port = 3336;
    }
    
    he = gethostbyname(hostname);
    if (he == NULL) {
        free(hostname);
        return;
    }
    
    dn_lock(&dn_fd_lock);
    for (curfd = 0; fds[curfd]; curfd++);
    fds[curfd] = socket(AF_INET, SOCK_STREAM, 0);
    dn_unlock(&dn_fd_lock);
    if (fds[curfd] == -1) {
        fds[curfd] = 0;
        perror("socket");
        return;
    }
    if (curfd > onfd) onfd = curfd;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    /*inet_aton(hostname, &inIP);
    addr.sin_addr = inIP;*/
    memset(&(addr.sin_zero), '\0', 8);

    if (connect(fds[curfd], (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        free(hostname);
        return;
    }

    onfd_ptr = malloc(sizeof(int));
    *onfd_ptr = curfd;
    pthread_attr_init(&ptattr);
    pthreadres = pthread_create(&pthreads[onpthread], &ptattr, communicator, (void *) onfd_ptr);

    onpthread++;
    
    free(hostname);
}
