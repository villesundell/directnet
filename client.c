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

    he = gethostbyname(destination);
    if (he == NULL) {
        //printf("Error: %d\nHOST_NOT_FOUND: %d\nNO_ADDRESS: %d\nNO_DATA: %d\nNO_RECOVERY: %d\nNO_RECOVERY: %d\n", h_errno, HOST_NOT_FOUND, NO_ADDRESS, NO_DATA, NO_RECOVERY, NO_RECOVERY);
        return;
    }
    
    curfd = onfd;
    onfd++;
    fds[curfd] = socket(AF_INET, SOCK_STREAM, 0);
    if (fds[curfd] == -1) {
        perror("socket");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3336);
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    /*inet_aton(destination, &inIP);
    addr.sin_addr = inIP;*/
    memset(&(addr.sin_zero), '\0', 8);

    if (connect(fds[curfd], (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        return;
    }

    onfd_ptr = malloc(sizeof(int));
    *onfd_ptr = curfd;
    pthread_attr_init(&ptattr);
    pthreadres = pthread_create(&pthreads[onpthread], &ptattr, communicator, (void *) onfd_ptr);

    onpthread++;
}
