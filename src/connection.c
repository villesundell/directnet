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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
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
    
    dn_lockInit(pipe_locks+fdnum);
    
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
                uiLoseConn(name);
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
    /*int cpied;
    strncpy(into, command, 65534);
    cpied = strlen(command);
    
    into[cpied+1] = vera;
    into[cpied+2] = verb;
    cpied += 2;
    
    strncpy(into + cpied, param, 65536 - cpied);*/
}

// Add a parameter into a command buffer
void addParam(char *into, char *newparam)
{
    sprintf(into+strlen(into), ";%s", newparam);
    /*int cpied = strlen(into);
    
    into[cpied] = ';';
    cpied++;
    
    strncpy(into + cpied, newparam, 65536-cpied);*/
}

// Send a command
void sendCmd(int fdnum, char *buf)
{
    dn_lock(pipe_locks+fdnum);
    send(fds[fdnum], buf, strlen(buf)+1, 0);
    dn_unlock(pipe_locks+fdnum);
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
    if ((!strncmp(command, "dcr", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) ||
        (!strncmp(command, "dce", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1)) {
        REQ_PARAMS(3);
        
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            if (!strncmp(command, "dcr", 3) &&
                inbuf[3] == 1 && inbuf[4] == 1) {
                char *outParams[50], hostbuf[256];
                struct hostent *he;
                
                memset(outParams, 0, 50 * sizeof(char *));
            
                // First, echo
                outParams[0] = hashSGet(dn_routes, params[1]);
                if (outParams[0] == NULL) {
                    return;
                }
                outParams[1] = dn_name;
                gethostname(hostbuf, 128);
                he = gethostbyname(hostbuf);
                outParams[2] = inet_ntoa(*((struct in_addr *)he->h_addr));
            
                handleRoutedMsg("dce", 1, 1, outParams);
            }
            
            // Then, attempt the connection
            establishClient(params[2]);
        }
    } else if (!strncmp(command, "fnd", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) {
        int remfd;
        char newroute[32256], ostrlen, outbuf[65536];
        
        REQ_PARAMS(4);
        
        if (!handleNLUnroutedMsg(params)) {
            return;
        }
        
        // Am I this person?
        if (!strncmp(params[2], dn_name, 1024)) {
            char *routeElement[50], *outparams[5];
            char reverseRoute[32256];
            int onRE, i, ostrlen;
            DN_LOCK *recFndLock;
            pthread_t curPthread;
            
            memset(routeElement, 0, 50 * sizeof(char *));
            memset(outparams, 0, 5 * sizeof(char *));
            
            // Get a fnd receive lock.  This is actually a lock on a hash of locks ... weird, I know
            dn_lock(&recFndHashLock);
        
            // Now that we have the hash locked, lock our particular lock in the hash (gack)
            recFndLock = hashLGet(recFndLocks, params[1]);
            dn_lock(recFndLock);
            
            // And give up the lock on the hash
            dn_unlock(&recFndHashLock);
            
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

            // Get the pthread
            curPthread = hashPGet(recFndPthreads, params[1]);
            
            // If we haven't started one, good
            if (curPthread == -1) {
                pthread_attr_t ptattr;
                void *route_voidptr;
                
                // This is the first fnd received, but ignore it if we already have a route
                if (hashSGet(dn_routes, params[1])) {
                    dn_unlock(recFndLock);
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
                
                uiEstRoute(params[1]);
                
                route_voidptr = malloc((strlen(params[1]) + 1) * sizeof(char));
                strcpy((char *) route_voidptr, params[1]);
                
                pthread_attr_init(&ptattr);
                pthread_create(&curPthread, &ptattr, fndPthread, route_voidptr);
                
                // Then put curPthread back where we found it
                hashPSet(recFndPthreads, params[1], curPthread);
            } else {
                char oldWRoute[32256];
                char *curWRoute;
                char *newRouteElements[50];
                int onRE, x, y, ostrlen;
                
                memset(newRouteElements, 0, 50 * sizeof(char *));
                
                curWRoute = hashSGet(weakRoutes, params[1]);
                // If there is no current weakroute, just copy in the current route
                if (!curWRoute) {
                    char *curroute;
                    
                    curroute = hashSGet(dn_routes, params[1]);
                    if (curroute == NULL) {
                        dn_unlock(recFndLock);
                        return;
                    }
                    
                    hashSSet(weakRoutes, params[1], curroute);
                    curWRoute = hashSGet(weakRoutes, params[1]);
                }
                
                strncpy(oldWRoute, curWRoute, 32256);
                
                newRouteElements[0] = oldWRoute;
                onRE = 1;
                
                ostrlen = strlen(oldWRoute);
                for (x = 0; x < ostrlen; x++) {
                    if (oldWRoute[x] == '\n') {
                        oldWRoute[x] = '\0';
                        
                        if (oldWRoute[x+1] != '\0') {
                            newRouteElements[onRE] = oldWRoute+x+1;
                            onRE++;
                        }
                    }
                }
                
                // Now compare the two into a new one
                curWRoute[0] = '\0';
                
                for (x = 0; newRouteElements[x] != NULL; x++) {
                    for (y = 0; routeElement[y] != NULL; y++) {
                        if (!strncmp(newRouteElements[x], routeElement[y], 24)) {
                            // It's a match, copy it in.
                            ostrlen = strlen(curWRoute);
                            strncpy(curWRoute+ostrlen, routeElement[y], 32256-ostrlen);
                            ostrlen += strlen(routeElement[y]);
                            strncpy(curWRoute+ostrlen, "\n", 32256-ostrlen);
                        }
                    }
                }
                
                // And put it back in the hash
                hashSSet(weakRoutes, params[1], curWRoute);
            }
            
            dn_unlock(recFndLock);
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
        char handleit;
        
        REQ_PARAMS(4);
        
        handleit = handleRoutedMsg(command, inbuf[3], inbuf[4], params);
        
        if (!handleit) {
            // Don't continue the route, but do add intermediate routes
            int i;
            char *newroute, *endu, myn[256];
            char checknext, usenext;
            
            // Figure out the route from here to the end user
            for (i = 0; params[0][i] != '\n'; i++);
            newroute = params[0]+i+1;
            
            for (i = strlen(params[0])-2; params[0][i] != '\n'; i--);
            endu = params[0]+i+1;
            
            // And add an intermediate route
            hashSSet(dn_iRoutes, endu, newroute);
            
            // Then figure out the route from here back
            /*i = strncpy(myn, dn_name, 256);
            strncpy(myn + i, "\n", 256-i);*/
            sprintf(myn, "%.254s\n", dn_name);
            
            // Loop through to find my name
            checknext = 1;
            usenext = 0;
            for (i = 0; params[2][i] != '\0'; i++) {
                if (checknext) {
                    checknext = 0;
                    // Hey, it's me, the next name is my route
                    if (!strncmp(params[2]+i, myn, strlen(myn))) {
                        usenext = 1;
                    }
                }
                
                if (params[2][i] == '\n') {
                    if (usenext) {
                        newroute = params[2]+i+1;
                        break;
                    }
                }
            }
            
            for (i = strlen(params[2])-2; params[2][i] != '\n'; i--);
            endu = params[2]+i+1;
            
            // And add that route
            hashSSet(dn_iRoutes, endu, newroute);
        } else {
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
            
            uiEstRoute(params[1]);
        }
    } else if (!strncmp(command, "key", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
#ifndef AIX
        char route[strlen(params[0])+2];
#else
        char *route;
#endif
        
        REQ_PARAMS(2);
        
#ifdef AIX
        route = (char *) malloc((strlen(params[0])+2) * sizeof(char));
#endif
        
        hashSet(dn_fds, params[0], fdnum);
        
        sprintf(route, "%s\n", params[0]);
        hashSSet(dn_routes, params[0], route);
        dn_route_by_num[fdnum] = hashSGet(dn_routes, params[0]);
        
#ifdef AIX
        free(route);
#endif
        
        gpgImportKey(params[1]);
        
        uiEstRoute(params[0]);
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
void *fndPthread(void *name_voidptr)
{
    char name[256];
    char isWeak = 0;
    char *curWRoute;
    
    strncpy(name, (char *) name_voidptr, 256);
    free(name_voidptr);
    
    sleep(15);
    
    /* FIXME
    this should lock the recFndLocks hash */
    
    // Figure out if there's a weak route
    curWRoute = hashSGet(weakRoutes, name);
    if (curWRoute) {
        // If the string has a location, check if it's weak
        if (strlen(curWRoute) > 0) {
            isWeak = 1;
        }
        hashSDelKey(weakRoutes, name);
    } else {
        // If this isn't set, that means _NO_ other fnd was received, VERY weak route!
        isWeak = 1;
    }
    
    if (!isWeak) {
        hashPSet(recFndPthreads, name, -1);
        return NULL;
    }
    
    // If it's weak, send a dcr (direct connect request)
    {
        char *params[50], hostbuf[128], *ip, *route;
        struct hostent *he;
        
        memset(params, 0, 50 * sizeof(char *));
        
        route = hashSGet(dn_routes, name);
        params[0] = (char *) malloc((strlen(route) + 1) * sizeof(char));
        strcpy(params[0], route);
        params[1] = dn_name;
        
        gethostname(hostbuf, 128);
        he = gethostbyname(hostbuf);
        ip = inet_ntoa(*((struct in_addr *)he->h_addr));
        params[2] = (char *) malloc((strlen(ip) + 1) * sizeof(char));
        strcpy(params[2], ip);

        handleRoutedMsg("dcr", 1, 1, params);
        
        free(params[0]);
        free(params[2]);
        
        hashPSet(recFndPthreads, name, -1);
        return NULL;
    }
}

/* Commands used by the UI */
void establishConnection(char *to)
{
    char *route;
    
    route = hashSGet(dn_routes, to);
    if (route) {
        char *params[50], *ip, hostbuf[128];
        struct hostent *he;
        
        memset(params, 0, 50 * sizeof(char *));
        
        // It's a user, send a DCR
        params[0] = (char *) malloc((strlen(route) + 1) * sizeof(char));
        strcpy(params[0], route);

        params[1] = dn_name;
 
        gethostname(hostbuf, 128);
        he = gethostbyname(hostbuf);
        ip = inet_ntoa(*((struct in_addr *)he->h_addr));
        params[2] = (char *) malloc((strlen(ip) + 1) * sizeof(char));
        strcpy(params[2], ip);
        
        handleRoutedMsg("dcr", 1, 1, params);
        
        free(params[0]);
        free(params[2]);
    } else {
        // It's a hostname or IP
        establishClient(to);
    }
}

int sendMsg(char *to, char *msg)
{
    char *outparams[50], *route;
    
    memset(outparams, 0, 50 * sizeof(char *));
        
    route = hashSGet(dn_routes, to);
    if (route == NULL) {
        uiNoRoute(to);
        return 0;
    }
    outparams[0] = (char *) malloc((strlen(route)+1) * sizeof(char));
    strcpy(outparams[0], route);
    outparams[1] = dn_name;
    outparams[2] = gpgTo(dn_name, to, msg);
    handleRoutedMsg("msg", 1, 1, outparams);
        
    free(outparams[0]);
    
    return 1;
}

void sendFnd(char *to)
{
    char outbuf[65536];
    
    // Find a user by name
    buildCmd(outbuf, "fnd", 1, 1, "");
    addParam(outbuf, dn_name);
    addParam(outbuf, to);
    addParam(outbuf, gpgExportKey());
        
    emitUnroutedMsg(-1, outbuf);
}
