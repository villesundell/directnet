/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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

#include <map>
#include <string>
using namespace std;

extern "C" {
#ifndef WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "connection.h"
#include "directnet.h"
#include "client.h"
#include "dn_event.h"
extern "C" {
#include <errno.h>
}

struct outgc {
    string outgh;
    int outgp;
};

void connect_act(dn_event_fd *ev, int cond);

void async_establishClient(const string &destination)
{
    int flags, ret;
    struct hostent *he;
    struct sockaddr_in addr;
    //struct in_addr inIP;
    string hostname;
    int i, port, fd;
#ifdef __WIN32
    unsigned long nblock;
#endif
    
    /* split 'destination' into 'hostname' and 'port' */
    hostname = destination;
    i = hostname.find_first_of(':');
    if (i != string::npos) {
        /* it has a port */
        port = atoi(hostname.substr(i + 1).c_str());
        hostname = hostname.substr(0, i);
    } else {
        port = 3447;
    }
    
    he = gethostbyname(hostname.c_str());
    if (he == NULL) {
        return;
    }
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return;
    }
#ifndef __WIN32
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    nblock = 1;
    ioctlsocket(fd, FIONBIO, &nblock);
#endif
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    /*inet_aton(hostname, &inIP);
    addr.sin_addr = inIP;*/
#ifndef NESTEDVM
    memset(&(addr.sin_zero), '\0', 8);
#endif
    
    ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
#ifndef NESTEDVM
    if (
#ifndef __WIN32
        errno != EINPROGRESS
#else
        ret == 0 || WSAGetLastError() != WSAEWOULDBLOCK
#endif
        ) {
        perror("connect()");
        close(fd);
        return;
    }
#endif
    
    outgc *ogc = new outgc;
    ogc->outgh = hostname;
    ogc->outgp = port;
    
    dn_event_fd *ev = new dn_event_fd(fd, DN_EV_READ | DN_EV_WRITE, connect_act, (void*)ogc);
    ev->activate();
}

void connect_act(dn_event_fd *ev, int cond) {
    int fd = ev->getFD();
    outgc *ogc = (outgc *)ev->payload;
    delete ev;
    
    char dummy;
    int ret;
    
#if 0
    ret = recv(fd, &dummy, 0, 0);
    if (ret < 0) {
        perror("async connect");
        close(fd);
        return;
    }
#endif
    
    init_comms(fd, &ogc->outgh, ogc->outgp);
    delete ogc;
}
