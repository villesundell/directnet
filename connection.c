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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection.h"
#include "directnet.h"
#include "gpg.h"
#include "ui.h"

void handleMsg(char *inbuf, int fdnum);
int handleUnroutedMsg(char **params);

void *communicator(void *fdnum_voidptr)
{
    int fdnum, byterec, origstrlen;
    char buf[65536];
    
    fdnum = *((int *) fdnum_voidptr);
    free(fdnum_voidptr);
    
    buildCmd(buf, "key", 1, 1, dn_name);
    addParam(buf, gpgExportKey());
    sendCmd(fdnum, buf);
    
    buf[0] = '\0';
    
    // Input loop
    while (1) {
        origstrlen = strlen(buf);
        buf[origstrlen+1] = '\0';
        byterec = recv(fds[fdnum], buf+origstrlen, 1, 0);
        if (byterec == 1) {
            if  (buf[origstrlen] == '\0') {
                // We just received a \0, the message is over
                handleMsg(buf, fdnum);
                buf[0] = '\0';
            }
        } else if (byterec == 0) {
            // Disconnect
            close(fds[fdnum]);
            return NULL;
        } else if (byterec == -1) {
            perror("recv");
            return (void *) -1;
        }
    }
    
    return NULL;
}

void buildCmd(char *into, char *command, char vera, char verb, char *param)
{
    sprintf(into, "%s%c%c%s", command, vera, verb, param);
}

void addParam(char *into, char *newparam)
{
    sprintf(into+strlen(into), ";%s", newparam);
}

void sendCmd(int fdnum, char *buf)
{
    send(fds[fdnum], buf, strlen(buf)+1, 0);
}

#define REQ_PARAMS(x) if (params[x-1] == NULL) return

void handleMsg(char *inbuf, int fdnum)
{
    char command[4], *params[50];
    int i, onparam, ostrlen;
    
    strncpy(command, inbuf, 3);
    command[3] = '\0';
    
    memset(params, 0, 50 * sizeof(char *));

    params[0] = inbuf+5;
    onparam = 1;
    
    ostrlen = strlen(inbuf);
    for (i = 3; i < ostrlen; i++) {
        if (inbuf[i] == ';') {
            inbuf[i] = '\0';
            params[onparam] = inbuf+i+1;
            onparam++;
        }
    }
    
    // Current protocol commands
    if (!strncmp(command, "fnd", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) {
        int remfd;
        char newroute[32256], ostrlen;
        
        REQ_PARAMS(4);
        
        if (!handleUnroutedMsg(params)) {
            return;
        }
        
        // Am I this person?
        if (!strncmp(params[1], dn_name, 1024)) {
            printf("Got a fnd for me!  Handle here!\n");
        }
        
        // Add myself to the route
        strncpy(newroute, params[2], 32256);
        ostrlen = strlen(newroute);
        strncpy(newroute+ostrlen, dn_name, 32256-ostrlen);
        ostrlen = strlen(newroute);
        strncpy(newroute+ostrlen, "\n", 32256-ostrlen);

        char outbuf[65536];
        buildCmd(outbuf, "fnd", 1, 1, params[0]);
        addParam(outbuf, params[1]);
        addParam(outbuf, newroute);
        addParam(outbuf, params[3]);
        
        // Do I have a connection to this person?
        remfd = hashGet(dn_fds, params[1]);
        if (remfd != -1) {
            sendCmd(remfd, outbuf);
            return;
        }
        
        // If all else fails, continue the chain
        emitUnroutedMsg(fdnum, outbuf);
    } else if (!strncmp(command, "key", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) {
        char route[strlen(params[0])+2];
        
        REQ_PARAMS(2);
        
        hashSet(dn_fds, params[0], fdnum);
        
        sprintf(route, "%s\n", params[0]);
        hashSSet(dn_routes, params[0], route);
        
        gpgImportKey(params[1]);
    } else if (!strncmp(command, "msg", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        REQ_PARAMS(3);
        
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            // This is our message
            uiDispMsg(params[1], gpgFrom(dn_name, params[2]));
            return;
        }
    }
}

int handleRoutedMsg(char *command, char vera, char verb, char **params)
{
    int i, sendfd;
    char *newroute, newbuf[65536];
    
    if (strlen(params[0]) == 0) {
        return 1; // This is our data
    }
    
    for (i = 0; params[0][i] != '\n'; i++);
    params[0][i] = '\0';
    newroute = params[0]+i+1;
        
    sendfd = hashGet(dn_fds, params[0]);
        
    if (sendfd == -1) {
        return 0;
    }
    
    buildCmd(newbuf, command, vera, verb, newroute);
    for (i = 1; params[i] != NULL; i++) {
        addParam(newbuf, params[i]);
    }
        
    sendCmd(sendfd, newbuf);
    
    return 0;
}

int handleUnroutedMsg(char **params)
{
    // This part of handleUnroutedMsg decides whether to just delete it
    if (hashGet(dn_trans_keys, params[0]) == -1) {
        hashSet(dn_trans_keys, params[0], 1);
        return 1;
    }
    return 0;
}

void emitUnroutedMsg(int fromfd, char *outbuf)
{
    // This emits an unrouted message
    int curfd;

    for (curfd = 0; curfd < onfd; curfd++) {
        if (curfd != fromfd) {
            sendCmd(curfd, outbuf);
        }
    }
}
