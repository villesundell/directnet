/*
 * Copyright 2004, 2005  Gregor Richards
 * Copyright 2006  Bryan Donlan
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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

extern "C" {
#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

#include "directnet/client.h"
#include "directnet/connection.h"
#include "directnet/directnet.h"
#include "directnet/server.h"
#include "directnet/ui.h"
#include "directnet/dn_event.h"

namespace DirectNet {
    //void *serverAcceptLoop(void *ignore);
    static void serverActivity(dn_event_fd *ev, int cond);
    static void serverAccept(int fd);

    /* Socket setup (just for Windows) */
    static void initSockets()
    {
#ifdef WIN32
        static BOOL sockets_init = FALSE;
        WSADATA dw;
        if (sockets_init) return;
        if (WSAStartup(MAKEWORD(1,1), &dw) != 0) { /*check for version 1.1 */
            perror("WSAStartup - initializing winsock failed");
            exit(-1);
        }
        sockets_init = TRUE;
#endif
    }

    /* Set up the server
     * returns the event for the server or NULL if the server wasn't started */
    dn_event_fd *establishServer()
    {
        int server_sock;
        int yes=1;
        int flags;
        dn_event_fd *listen_ev;
#ifdef __WIN32
        unsigned long nblock;
#endif
    
        struct sockaddr_in addr;
        initSockets();

        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock == -1) {
            perror("socket");
            exit(-1);
        }
    
        if (setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,(char *) &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(-1);
        }
    
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serv_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifndef NESTEDVM
        memset(&(addr.sin_zero), '\0', 8);
#endif
    
        if (bind(server_sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
            perror("bind");
            return 0;
        }
    
        if (listen(server_sock, 10) == -1) {
            perror("listen");
            exit(-1);
        }
    
#ifndef __WIN32
        flags = fcntl(server_sock, F_GETFL);
        fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
#else
        nblock = 1;
        ioctlsocket(server_sock, FIONBIO, &nblock);
#endif
    
        listen_ev = new dn_event_fd(server_sock, DN_EV_READ|DN_EV_EXCEPT, serverActivity);
        listen_ev->activate();
    
        return listen_ev;
    }

    /* The receiver for activity on the server port */
    static void serverActivity(dn_event_fd *ev, int cond) {
        if (cond & DN_EV_EXCEPT) {
            /*fprintf(stderr, "Our server socket seems to have imploded. That sounds fun; I'll do it too.\n");
            abort();*/
            return;
        }
        serverAccept(ev->getFD());
    }

    /* Accept a connection from the server */
    static void serverAccept(int server_sock) {
        int sin_size;
        int acceptfd;
        struct sockaddr_in rem_addr;
        int flags;
#ifdef __WIN32
        unsigned long nblock;
#endif
    
        //acceptfd = accept(server_sock, (struct sockaddr *)&rem_addr, (unsigned int *) &sin_size);
        acceptfd = accept(server_sock, NULL, NULL);
        /*fprintf(stderr, "accept=%d\n", acceptfd);*/
        if (acceptfd < 0) {
            perror("accept failed");
            return;
        }
    
        // make it non-blocking
#ifndef __WIN32
        flags = fcntl(acceptfd, F_GETFL);
        fcntl(acceptfd, F_SETFL, flags | O_NONBLOCK);
#else
        nblock = 1;
        ioctlsocket(acceptfd, FIONBIO, &nblock);
#endif
    
        init_comms(acceptfd, NULL, 0);
    }
}
