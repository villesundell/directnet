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
int handleNLUnroutedMsg(char **params);
int handleNRUnroutedMsg(char **params);

void *communicator(void *fdnum_voidptr)
{
    int fdnum, byterec, origstrlen;
    char buf[65536];
    
    fdnum = *((int *) fdnum_voidptr);
    free(fdnum_voidptr);
    
    // Immediately send our key
    buildCmd(buf, "key", 1, 1, dn_name);
    addParam(buf, gpgExportKey());
    sendCmd(fdnum, buf);
    
    buf[0] = '\0';
    
    // Input loop
    while (1) {
        // Receive a byte at a time
        origstrlen = strlen(buf);
        buf[origstrlen+1] = '\0';
        
        byterec = recv(fds[fdnum], buf+origstrlen, 1, 0);
        
        if (byterec == 1) {
            if  (buf[origstrlen] == '\0') {
                // We just received a \0, the message is over, so handle it
                handleMsg(buf, fdnum);
                buf[0] = '\0';
            }
        } else if (byterec == 0) {
            char *name;
            
            // Disconnect
            name = hashRevGet(dn_fds, fdnum);
            if (name != NULL) {
                uiDispMsg(name, "Lost connection.");
                hashSDelKey(dn_routes, name);
            }
            
            close(fds[fdnum]);
            return NULL;
        } else if (byterec == -1) {
            perror("recv");
            return (void *) -1;
        }
    }
    
    return NULL;
}

// Start building a command into a buffer
void buildCmd(char *into, char *command, char vera, char verb, char *param)
{
    sprintf(into, "%s%c%c%s", command, vera, verb, param);
}

// Add a parameter into a command buffer
void addParam(char *into, char *newparam)
{
    sprintf(into+strlen(into), ";%s", newparam);
}

// Send a command
void sendCmd(int fdnum, char *buf)
{
    send(fds[fdnum], buf, strlen(buf)+1, 0);
}

void *fndPthread(void *fdnum_voidptr);

#define REQ_PARAMS(x) if (params[x-1] == NULL) return

void handleMsg(char *inbuf, int fdnum)
{
    char command[4], *params[50];
    int i, onparam, ostrlen;
    
    // Get the command itself
    strncpy(command, inbuf, 3);
    command[3] = '\0';
    
    memset(params, 0, 50 * sizeof(char *));
    
    params[0] = inbuf+5;
    onparam = 1;
    
    // Divide up the buffer by ;, getting the parameters
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
        char newroute[32256], ostrlen, outbuf[65536];;
        
        REQ_PARAMS(4);
        
        if (!handleNLUnroutedMsg(params)) {
            return;
        }
        
        // Am I this person?
        if (!strncmp(params[2], dn_name, 1024)) {
            char *routeElement[50], *outparams[5];
            char reverseRoute[32256];
            int onRE, i, ostrlen;
            
            memset(routeElement, 0, 50 * sizeof(char *));
            memset(outparams, 0, 5 * sizeof(char *));
            
            // Start with the current route
            strncpy(newroute, params[0], 32256);
            
            // Then tokenize it by \n
            routeElement[0] = newroute;
            onRE = 1;
            
            ostrlen = strlen(newroute);
            for (i = 0; i < ostrlen; i++) {
                if (newroute[i] == '\n') {
                    newroute[i] = '\0';
                    if (newroute[i+1] != '\0') {
                        routeElement[onRE] = newroute+i+1;
                        onRE++;
                    }
                }
            }
            
            if (!fnd_pthreads[fdnum]) {
                // This is the first fnd received, but ignore it if we already have a route
                if (dn_route_by_num[fdnum]) {
                    return;
                }
                
                // If we haven't already gotten the fastest route, this must be it
                onRE--;
            
                // Build backwards route
                reverseRoute[0] = '\0';
                for (i = onRE; i >= 0; i--) {
                    ostrlen = strlen(reverseRoute);
                    strncpy(reverseRoute+ostrlen, routeElement[i], 32256-ostrlen);
                    ostrlen += strlen(routeElement[i]);
                    strncpy(reverseRoute+ostrlen, "\n", 32256-ostrlen);
                }
                ostrlen = strlen(reverseRoute);
                strncpy(reverseRoute+ostrlen, params[1], 32256-ostrlen);
                ostrlen += strlen(params[1]);
                strncpy(reverseRoute+ostrlen, "\n", 32256-ostrlen);
                
                // Add his route,
                hashSSet(dn_routes, params[1], reverseRoute);
                dn_route_by_num[fdnum] = hashSGet(dn_routes, params[1]);
            
                // and public key,
                gpgImportKey(params[3]);
                
                // then send your route back to him
                outparams[0] = reverseRoute;
                outparams[1] = dn_name;
                outparams[2] = params[0];
                outparams[3] = gpgExportKey();
                handleRoutedMsg("fnr", 1, 1, outparams);
                
                uiDispMsg(params[1], "Route established.");
                
                // FIXME : this should start up the harvesting pthread
            } else {
                /* FIXME : this should compare the current weak-checking route to the just-received
                   route */
            }
            
            return;
        }
        
        // Add myself to the route
        strncpy(newroute, params[0], 32256);
        ostrlen = strlen(newroute);
        strncpy(newroute+ostrlen, dn_name, 32256-ostrlen);
        ostrlen = strlen(newroute);
        strncpy(newroute+ostrlen, "\n", 32256-ostrlen);

        buildCmd(outbuf, "fnd", 1, 1, newroute);
        addParam(outbuf, params[1]);
        addParam(outbuf, params[2]);
        addParam(outbuf, params[3]);
        
        // Do I have a connection to this person?
        remfd = hashGet(dn_fds, params[2]);
        if (remfd != -1) {
            sendCmd(remfd, outbuf);
            return;
        }
        
        // If all else fails, continue the chain
        emitUnroutedMsg(fdnum, outbuf);
    } else if (!strncmp(command, "fnr", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        REQ_PARAMS(4);
        
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            char newroute[32256];
            int ostrlen;
            
            // Hoorah, add a user
            
            // 1) Route
            strncpy(newroute, params[2], 32256);
            
            ostrlen = strlen(newroute);
            strncpy(newroute+ostrlen, params[1], 32256-ostrlen);
            ostrlen += strlen(params[1]);
            strncpy(newroute+ostrlen, "\n", 32256-ostrlen);
            
            hashSSet(dn_routes, params[1], newroute);
            
            // 2) Key
            gpgImportKey(params[3]);
            
            uiDispMsg(params[1], "Route established.");
        }
    } else if (!strncmp(command, "key", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        char route[strlen(params[0])+2];
        
        REQ_PARAMS(2);
        
        hashSet(dn_fds, params[0], fdnum);
        
        sprintf(route, "%s\n", params[0]);
        hashSSet(dn_routes, params[0], route);
        
        gpgImportKey(params[1]);
        
        uiDispMsg(params[0], "Route established.");
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

int handleNLUnroutedMsg(char **params)
{
    char origroute[32256];
    char *divroute[50];
    int ondr, i, ostrlen;
    
    memset(divroute, 0, 50 * sizeof(char *));
    
    strncpy(origroute, params[0], 32256);
    
    divroute[0] = origroute;
    ondr = 1;
    
    ostrlen = strlen(origroute);
    for (i = 0; i < ostrlen; i++) {
        if (origroute[i] == '\n') {
            origroute[i] = '\0';
            if (origroute[i+1] != '\0') {
                divroute[ondr] = origroute+i+1;
                ondr++;
            }
        }
    }
    
    for (i = ondr - 1; i >= 0; i--) {
        if (!strncmp(dn_name, divroute[i], 24)) {
            return 0;
        }
    }
    
    return 1;
}

int handleNRUnroutedMsg(char **params)
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

// fndPthread is the pthread that harvests late fnds
void *fndPthread(void *fdnum_voidptr)
{
    int fdnum;
    char isWeak = 0;
    
    fdnum = *((int *) fdnum_voidptr);
    free(fdnum_voidptr);
    
    sleep(15);
    
    // Figure out if there's a weak route
    if (weakRoutes[fdnum]) {
        // If the string has a location, check if it's weak
        if (strlen(weakRoutes[fdnum]) > 0) {
            isWeak = 1;
        }
        free(weakRoutes[fdnum]);
        weakRoutes[fdnum] = NULL;
    } else {
        // If this isn't set, that means _NO_ other fnd was received, VERY weak route!
        isWeak = 1;
    }
    
    if (!isWeak) {
        fnd_pthreads[fdnum] = 0;
        return NULL;
    }
    
    // If it's weak, send a dcr (direct connect request)
    {
        char outbuf[65536];
        
        buildCmd(outbuf, "dcr", 1, 1, dn_route_by_num[fdnum]);
        addParam(outbuf, dn_name);
        addParam(outbuf, "0.0.0.0"); // FIXME
        
        //sendCmd(fdnum, outbuf);
        
        return NULL;
    }
}
