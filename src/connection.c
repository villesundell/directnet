/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "auth.h"
#include "chat.h"
#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "enc.h"
#include "hash.h"
#include "ui.h"

char *awayMsg = NULL;

void handleMsg(const char *inbuf, int fdnum);
int handleNLUnroutedMsg(char **params);
int handleNRUnroutedMsg(int fromfd, const char *command, char vera, char verb, char **params);
int sendMsgB(const char *to, const char *msg, char away, char sign);
void recvFnd(char *route, const char *name, const char *key);

void *communicator(void *fdnum_voidptr)
{
    int fdnum, pthreadnum, byterec, origstrlen;
    char buf[DN_CMD_LEN];
    int connCount, curfd;
    
    fdnum = ((struct communInfo *) fdnum_voidptr)->fdnum;
    pthreadnum = ((struct communInfo *) fdnum_voidptr)->pthreadnum;
    free(fdnum_voidptr);
    
    dn_lockInit(pipe_locks+fdnum);
    
    // Immediately send our name and key unless it's not set
    while (!dn_name_set) sleep(0);
    buildCmd(buf, "key", 1, 1, dn_name);
    addParam(buf, encExportKey());
    sendCmd(fdnum, buf);
    
    // Count the number of connections to see if we should promote
    /* TODO: This should have a timer: don't promote if you've been down from
     * promotion for under ten minutes */
    dn_lock(&dn_fd_lock);
    for (curfd = 0; curfd < onfd; curfd++)
        if (fds[curfd]) connCount++;
    dn_unlock(&dn_fd_lock);
    
    if (connCount > 4) {
        // TODO: promote();
    }
    
    buf[0] = '\0';
    
    // Input loop
    while (1) {
        // Receive a byte at a time
        origstrlen = strlen(buf);
        if (origstrlen >= DN_CMD_LEN - 1) origstrlen = DN_CMD_LEN - 1;
        buf[origstrlen+1] = '\0';
        
        byterec = recv(fds[fdnum], buf+origstrlen, 1, 0);
        
        if (byterec == 1) {
            if  (buf[origstrlen] == '\0') {
                // We just received a \0, the message is over, so handle it
                handleMsg(buf, fdnum);
                buf[0] = '\0';
            }
        } else if (byterec <= 0) {
            char *name;
            
            if (byterec == -1) {
                perror("recv");
            }
            
            // Disconnect
            name = hashIRevGet(dn_fds, fdnum);
            if (name != NULL) {
                uiLoseConn(name);
                hashSDelKey(dn_routes, name);
                hashISet(dn_fds, name, -1);
            }
            
            close(fds[fdnum]);
            fds[fdnum] = 0;
            break;
        }
    }
    
    dn_lock(&pthread_lock);
    free(pthreads[pthreadnum]);
    pthreads[pthreadnum] = NULL;
    dn_unlock(&pthread_lock);
    
    return NULL;
}

// Start building a command into a buffer
void buildCmd(char *into, const char *command, char vera, char verb, const char *param)
{
    /*sprintf(into, "%s%c%c%s", command, vera, verb, param);*/
    SF_strncpy(into, command, 4);
    into[3] = vera;
    into[4] = verb;
    SF_strncpy(into + 5, param, DN_CMD_LEN - 5);
}

// Add a parameter into a command buffer
void addParam(char *into, char *newparam)
{
    /*sprintf(into+strlen(into), ";%s", newparam);*/
    into[DN_CMD_LEN - 2] = '\0';
    int cpied = strlen(into);
    
    into[cpied] = ';';
    cpied++;
    
    strncpy(into + cpied, newparam, DN_CMD_LEN - cpied);
}

// Send a command
void sendCmd(int fdnum, const char *buf)
{
    dn_lock(pipe_locks+fdnum);
    if (fds[fdnum]) {
        send(fds[fdnum], buf, strlen(buf)+1, 0);
    }
    dn_unlock(pipe_locks+fdnum);
}

void *fndPthread(void *fdnum_voidptr);

#define REQ_PARAMS(x) if (params[x-1] == NULL) return
#define REJOIN_PARAMS(x) { \
	int i; \
	for (i = x; params[i] != NULL; i++) { \
		*(params[i]-1) = ';'; \
		params[i] = NULL; \
	} \
}
			
void handleMsg(const char *rdbuf, int fdnum)
{
    char command[4], *params[DN_MAX_PARAMS];
    char *inbuf;
    int i, onparam, ostrlen;
    
    // Get the command itself
    inbuf = alloca(strlen(rdbuf)+1);
    if (inbuf == NULL) { perror("strdup"); exit(1); }
    strcpy(inbuf, rdbuf);
    strncpy(command, inbuf, 3);
    command[3] = '\0';
    
    memset(params, 0, DN_MAX_PARAMS * sizeof(char *));
    
    params[0] = inbuf + 5;
    
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
    
    /* DEBUG * /
    printf("Command: %s\n", command);
    for (i = 0; params[i]; i++) {
        printf("Param %d: %s\n", i, params[i]);
    }
    putchar('\n');
    // */
    
    /*****************************
     * Current protocol commands *
     *****************************/
    
    if ((!strncmp(command, "cjo", 3) &&
         inbuf[3] == 1 && inbuf[4] == 1) ||
        (!strncmp(command, "con", 3) &&
         inbuf[3] == 1 && inbuf[4] == 1)) {
        REQ_PARAMS(3);
        
        // "I'm on this chat"
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            // Are we even on this chat?
            if (!chatOnChannel(params[1])) {
                return;
            }
            
            // OK, this person is on our list for this chat
            chatAddUser(params[1], params[2]);
            
            // Should we echo?
            if (!strncmp(command, "cjo", 3)) {
                char *outParams[DN_MAX_PARAMS];
                
                memset(outParams, 0, DN_MAX_PARAMS * sizeof(char *));
            
                outParams[0] = hashSGet(dn_routes, params[2]);
                if (outParams[0] == NULL) {
                    return;
                }
                outParams[1] = params[1];
                outParams[2] = dn_name;
                
                handleRoutedMsg("con", 1, 1, outParams);
            }
        }
        
    } else if ((!strncmp(command, "cms", 3) &&
         inbuf[3] == 1 && inbuf[4] == 1)) {
        REQ_PARAMS(6);
        
        // a chat message
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            char **users;
            char *outParams[DN_MAX_PARAMS];
            char *unenmsg;
            int i;
            
            // Should we just ignore it?
            dn_lock(&dn_transKey_lock);
            if (hashIGet(dn_trans_keys, params[1]) == -1) {
                hashISet(dn_trans_keys, params[1], 1);
                dn_unlock(&dn_transKey_lock);
            } else {
                dn_unlock(&dn_transKey_lock);
                return;
            }
        
            // Are we on this chat?
            if (!chatOnChannel(params[3])) {
                return;
            }
            
            // Yay, chat for us!
            
            // Decrypt it
            unenmsg = encFrom(params[2], dn_name, params[5]);
            if (unenmsg[0] == '\0') {
                free(unenmsg);
                return;
            }
            uiDispChatMsg(params[3], params[4], unenmsg);
            
            // Pass it on
            users = chatUsers(params[3]);
            
            memset(outParams, 0, DN_MAX_PARAMS * sizeof(char *));
            outParams[1] = params[1];
            outParams[2] = dn_name;
            outParams[3] = params[3];
            outParams[4] = params[4];
            
            for (i = 0; users[i]; i++) {
                outParams[0] = hashSGet(dn_routes, users[i]);
                if (outParams[0] == NULL) {
                    continue;
                }
                
                outParams[5] = encTo(dn_name, users[i], unenmsg);
                
                handleRoutedMsg("cms", 1, 1, outParams);
                
                free(outParams[5]);
            }
            
            free(unenmsg);
        }
        
    } else if ((!strncmp(command, "dcr", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) ||
        (!strncmp(command, "dce", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1)) {
        REQ_PARAMS(3);
        
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            // This doesn't work on OSX
#ifndef __APPLE__
            if (!strncmp(command, "dcr", 3) &&
                inbuf[3] == 1 && inbuf[4] == 1) {
                // dcr echos
                char *outParams[DN_MAX_PARAMS];
                char *rfe, *first;
                int firfd, locip_len;
                struct sockaddr locip;
                struct sockaddr_in *locip_i;
                
                memset(outParams, 0, DN_MAX_PARAMS * sizeof(char *));
            
                // First, echo, then try to connect
                outParams[0] = hashSGet(dn_routes, params[1]);
                if (outParams[0] == NULL) {
                    return;
                }
                outParams[1] = dn_name;
                
                // grab before the first \n for the next name in the line
                rfe = strchr(outParams[0], '\n');
                if (rfe == NULL) return;
                first = (char *) strdup(outParams[0]);
                first[rfe - outParams[0]] = '\0';
                // then get the fd
                firfd = hashIGet(dn_fds, first);
                if (firfd == -1) return;
                firfd = fds[firfd];
                free(first);
                // then turn the fd into a sockaddr
                locip_len = sizeof(struct sockaddr);
                if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
                    locip_i = (struct sockaddr_in *) &locip;
                    /*params[2] = strdup(inet_ntoa(*((struct in_addr *) &(locip_i->sin_addr))));*/
                    outParams[2] = strdup(inet_ntoa(locip_i->sin_addr));
                } else {
                    return;
                }
                outParams[2] = realloc(outParams[2], strlen(outParams[2]) + 7);
                sprintf(outParams[2] + strlen(outParams[2]), ":%d", serv_port);
                
                handleRoutedMsg("dce", 1, 1, outParams);
                
                free(outParams[2]);
            }
            
            // Then, attempt the connection
            establishClient(params[2]);
#endif
        }

    } else if (!strncmp(command, "fnd", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) {
        int remfd;
        char newroute[DN_ROUTE_LEN], ostrlen, outbuf[DN_CMD_LEN];
        
        REQ_PARAMS(4);
        
        if (!handleNLUnroutedMsg(params)) {
            return;
        }
        
        // Am I this person?
        if (!strncmp(params[2], dn_name, DN_NAME_LEN)) {
            recvFnd(params[0], params[1], params[3]);
            return;
        }
        
        // Add myself to the route
        SF_strncpy(newroute, params[0], DN_ROUTE_LEN);
        ostrlen = strlen(newroute);
        SF_strncpy(newroute + ostrlen, dn_name, DN_ROUTE_LEN - ostrlen);
        ostrlen = strlen(newroute);
        SF_strncpy(newroute + ostrlen, "\n", DN_ROUTE_LEN - ostrlen);

        buildCmd(outbuf, "fnd", 1, 1, newroute);
        addParam(outbuf, params[1]);
        addParam(outbuf, params[2]);
        addParam(outbuf, params[3]);
        
        // Do I have a connection to this person?
        remfd = hashIGet(dn_fds, params[2]);
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
            // This isn't our route, but do add intermediate routes
            int i, ostrlen;
            char endu[DN_NAME_LEN+1], myn[DN_NAME_LEN+1], newroute[DN_ROUTE_LEN];
            char checknext, usenext;
            
            // Fix our pararms[0]
            for (i = 0; params[0][i] != '\0'; i++);
            params[0][i] = '\n';
            
            // Who's the end user?
            for (i = strlen(params[0])-2; params[0][i] != '\n' && params[0][i] != '\0' && i >= 0; i--);
            SF_strncpy(endu, params[0]+i+1, DN_NAME_LEN+1);
            endu[strlen(endu)-1] = '\0';
            
            // And add an intermediate route
            hashSSet(dn_iRoutes, endu, params[0]);
            
            // Then figure out the route from here back
            /*i = strncpy(myn, dn_name, DN_NAME_LEN);
            strncpy(myn + i, "\n", DN_NAME_LEN-i);*/
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
                        SF_strncpy(newroute, params[2]+i+1, DN_ROUTE_LEN);
                        break;
                    }
                    checknext = 1;
                }
            }
            
            ostrlen = strlen(newroute);
            SF_strncpy(newroute + ostrlen, params[1], DN_ROUTE_LEN - ostrlen);
            
            // And add that route
            hashSSet(dn_iRoutes, params[1], newroute);
        } else {
            char newroute[DN_ROUTE_LEN];
            int ostrlen;
            
            // Hoorah, add a user
            
            // 1) Route
            SF_strncpy(newroute, params[2], DN_ROUTE_LEN);
            
            ostrlen = strlen(newroute);
            SF_strncpy(newroute + ostrlen, params[1], DN_ROUTE_LEN - ostrlen);
            ostrlen += strlen(params[1]);
            SF_strncpy(newroute + ostrlen, "\n", DN_ROUTE_LEN - ostrlen);
            
            hashSSet(dn_routes, params[1], newroute);
            hashSSet(dn_iRoutes, params[1], newroute);
            
            // 2) Key
            encImportKey(params[1], params[3]);
            
            uiEstRoute(params[1]);
        }
    
    } else if (!strncmp(command, "key", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        char route[strlen(params[0])+2];
        
        REQ_PARAMS(2);
        
        // if I already have a route to this person, drop it
        if (hashSGet(dn_routes, params[0])) {
            hashSDelKey(dn_routes, params[0]);
        }
        
        // now accept the new FD
        hashISet(dn_fds, params[0], fdnum);
        
        sprintf(route, "%s\n", params[0]);
        hashSSet(dn_routes, params[0], route);
        hashSSet(dn_iRoutes, params[0], route);
        
        encImportKey(params[0], params[1]);
        
        uiEstRoute(params[0]);

    } else if (!strncmp(command, "lst", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        REQ_PARAMS(2);
        
        uiLoseRoute(params[1]);
    
    } else if ((!strncmp(command, "msg", 3) &&
                inbuf[3] == 1 && inbuf[4] == 1) ||
               (!strncmp(command, "msa", 3) &&
                inbuf[3] == 1 && inbuf[4] == 1)) {
        REQ_PARAMS(3);
	REJOIN_PARAMS(3);
        
        if (handleRoutedMsg(command, inbuf[3], inbuf[4], params)) {
            // This is our message
            char *unencmsg, *dispmsg, *sig, *sigm;
            int austat, iskey;
            
            // Decrypt it
            unencmsg = encFrom(params[1], dn_name, params[2]);
            
            // And verify the signature
            dispmsg = authVerify(unencmsg, &sig, &austat);
            
            // Make our signature tag
            iskey = 0;
            if (austat == -1) {
                free(unencmsg);
                sigm = strdup("n");
            } else if (austat == 0) {
                free(unencmsg);
                sigm = strdup("u");
            } else if (austat == 1 && sig) {
                free(unencmsg);
                sigm = (char *) malloc((strlen(sig) + 3) * sizeof(char));
                sprintf(sigm, "s %s", sig);
            } else if (austat == 1 && !sig) {
                free(unencmsg);
                sigm = strdup("s");
            } else if (austat == 2) {
                /* this IS a signature, totally different response */
                uiAskAuthImport(params[1], unencmsg, sig);
                iskey = 1;
                sigm = NULL;
            } else {
                free(unencmsg);
                sigm = strdup("?");
            }
            if (sig) free(sig);
            
            if (!iskey) {
                uiDispMsg(params[1], dispmsg, sigm, !strncmp(command, "msa", 3));
                free(sigm);
            }
            free(dispmsg);
            
            // Are we away?
            if (awayMsg && strncmp(command, "msa", 3)) {
                sendMsgB(params[1], awayMsg, 1, 1);
            }
            
            return;
        }
        
    } else if (!strncmp(command, "prm", 3) &&
               inbuf[3] == 1 && inbuf[4] == 1) {
        int level;
        
        REQ_PARAMS(5);
        
        /* TODO: a list of previous leaders should be kept, to make sure that
         * nobody tries to lead over and over */
        
        // before handling, should we resend this?
        level = atoi(params[4]);
        if (level <= 0) return; // must be at least 1
        
        else if (level == 1 && hashIGet(dn_trans_keys, params[0]) != -1)
            return; // no repeats
        
        else {
            level--;
            
            /* adjust the parameter to the new level
             * (this can safely be done with sprintf since the length is
             * guaranteed to be at worst the same) */
            sprintf(params[3], "%d", level);
            
            if (!handleNRUnroutedMsg(fdnum, command, inbuf[3], inbuf[4], params))
                return;
        }
        
        // receive it like a "find"
        recvFnd(params[1], params[2], params[3]);
        
        /* new promotions always outweight old ones, so simply ignore whoever
         * led before */
        dn_lock(&dn_leader_lock);
        strncpy(params[2], dn_leader, DN_NAME_LEN);
        dn_leader[DN_NAME_LEN] = '\0';
        dn_led = 1;
        dn_is_leader = 0;
        // TODO: start a don't-be-a-leader-until thread
        dn_unlock(&dn_leader_lock);
    }
}

int handleRoutedMsg(const char *command, char vera, char verb, char **params)
{
    int i, sendfd;
    char *route, *newroute, newbuf[DN_CMD_LEN];
    
    if (strlen(params[0]) == 0) {
        return 1; // This is our data
    }
    
    route = strdup(params[0]);
    
    for (i = 0; route[i] != '\n' && route[i] != '\0'; i++);
    route[i] = '\0';
    newroute = route+i+1;
    
    sendfd = hashIGet(dn_fds, route);
        
    if (sendfd == -1) {
        // We need to tell the other side that this route is dead!
        if (strncmp(command, "lst", 3)) {
            char *newparams[DN_MAX_PARAMS], *endu, *iroute;
            // Bouncing a lost could end up in an infinite loop, so don't
            
            // Fix our route
            for (i = 0; route[i] != '\0'; i++);
            route[i] = '\n';
            
            // Who's the end user?
            route[strlen(route)-1] = '\0';
            
            for (i = strlen(route)-1; i >= 0 && route[i] != '\n'; i--);
            endu = route+i+1;
            
            // If this is me, don't send the command, just display it
            if (!strncmp(params[1], dn_name, DN_NAME_LEN)) {
                uiLoseRoute(endu);
            } else {
                // Do we have an intermediate route?
                iroute = hashSGet(dn_iRoutes, params[1]);
                if (iroute == NULL) return 0;
            
                // Send the command
                newparams[0] = iroute;
                newparams[1] = endu;
                newparams[2] = NULL;
                handleRoutedMsg("lst", 1, 1, newparams);
            }
        }
        
        free(route);
        return 0;
    }
    
    buildCmd(newbuf, command, vera, verb, newroute);
    for (i = 1; params[i] != NULL; i++) {
        addParam(newbuf, params[i]);
    }
    
    sendCmd(sendfd, newbuf);
    
    free(route);
    return 0;
}

int handleNLUnroutedMsg(char **params)
{
    char origroute[DN_ROUTE_LEN];
    char *divroute[DN_MAX_PARAMS];
    int ondr, i, ostrlen;
    
    memset(divroute, 0, DN_MAX_PARAMS * sizeof(char *));
    
    SF_strncpy(origroute, params[0], DN_ROUTE_LEN);
    
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
        if (!strncmp(dn_name, divroute[i], DN_NAME_LEN)) {
            return 0;
        }
    }
    
    return 1;
}

int handleNRUnroutedMsg(int fromfd, const char *command, char vera, char verb, char **params)
{
    // This part of handleUnroutedMsg decides whether to just delete it
    if (hashIGet(dn_trans_keys, params[0]) == -1) {
        char outbuf[DN_CMD_LEN];
        int i;
        
        hashISet(dn_trans_keys, params[0], 1);
        
        // Continue the transmission
        buildCmd(outbuf, command, vera, verb, params[0]);
        for (i = 1; params[i]; i++) {
            addParam(outbuf, params[i]);
        }
        emitUnroutedMsg(fromfd, outbuf);
        
        return 1;
    }
    return 0;
}

void emitUnroutedMsg(int fromfd, const char *outbuf)
{
    // This emits an unrouted message
    int curfd;

    for (curfd = 0; curfd < onfd; curfd++) {
        if (curfd != fromfd) {
            sendCmd(curfd, outbuf);
        }
    }
}

// recvFnd handles the situation that we've just been "found"
void recvFnd(char *route, const char *name, const char *key)
{
    char *routeElement[DN_MAX_PARAMS], *outparams[5];
    char reverseRoute[DN_ROUTE_LEN];
    int onRE, i, ostrlen;
    DN_LOCK *recFndLock;
    pthread_t *curPthread;
    char newroute[DN_ROUTE_LEN];
    
    memset(routeElement, 0, DN_MAX_PARAMS * sizeof(char *));
    memset(outparams, 0, 5 * sizeof(char *));
    
    // Get a fnd receive lock.  This is actually a lock on a hash of locks ... weird, I know
    dn_lock(&recFndHashLock);
        
    // Now that we have the hash locked, lock our particular lock in the hash (gack)
    recFndLock = hashLGet(recFndLocks, name);
    dn_lock(recFndLock);
            
    // And give up the lock on the hash
    dn_unlock(&recFndHashLock);
            
    // Start with the current route
    SF_strncpy(newroute, route, DN_ROUTE_LEN);
            
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
    curPthread = hashPGet(recFndPthreads, name);
            
    // If we haven't started one, good
    if (curPthread == (pthread_t *) -1) {
        pthread_attr_t ptattr;
        void *route_voidptr;
                
        // This is the first fnd received, but ignore it if we already have a route
        if (hashSGet(dn_routes, name)) {
            dn_unlock(recFndLock);
            return;
        }
                
        // If we haven't already gotten the fastest route, this must be it
        onRE--;
                
        // Build backwards route
        reverseRoute[0] = '\0';
        for (i = onRE; i >= 0; i--) {
            ostrlen = strlen(reverseRoute);
            SF_strncpy(reverseRoute + ostrlen, routeElement[i], DN_ROUTE_LEN - ostrlen);
            ostrlen += strlen(routeElement[i]);
            SF_strncpy(reverseRoute + ostrlen, "\n", DN_ROUTE_LEN - ostrlen);
        }
        ostrlen = strlen(reverseRoute);
        SF_strncpy(reverseRoute + ostrlen, name, DN_ROUTE_LEN - ostrlen);
        ostrlen += strlen(name);
        SF_strncpy(reverseRoute + ostrlen, "\n", DN_ROUTE_LEN - ostrlen);
                
        // Add his route,
        hashSSet(dn_routes, name, reverseRoute);
        hashSSet(dn_iRoutes, name, reverseRoute);
            
        // and public key,
        encImportKey(name, key);
        
        // then send your route back to him
        outparams[0] = reverseRoute;
        outparams[1] = dn_name;
        outparams[2] = route;
        outparams[3] = encExportKey();
        handleRoutedMsg("fnr", 1, 1, outparams);
        
        uiEstRoute(name);
        
        route_voidptr = (void *) malloc((strlen(name) + 1) * sizeof(char));
        strcpy((char *) route_voidptr, name);
        
        pthread_attr_init(&ptattr);
        curPthread = (pthread_t *) malloc(sizeof(pthread_attr_t));
        pthread_create(curPthread, &ptattr, fndPthread, route_voidptr);
        
        // Put it back where it belongs
        hashPSet(recFndPthreads, name, curPthread);
    } else {
        char oldWRoute[DN_ROUTE_LEN];
        char *curWRoute;
        char *newRouteElements[DN_MAX_PARAMS];
        int onRE, x, y, ostrlen;
        
        memset(newRouteElements, 0, DN_MAX_PARAMS * sizeof(char *));
        
        curWRoute = hashSGet(weakRoutes, name);
        // If there is no current weakroute, just copy in the current route
        if (!curWRoute) {
            char *curroute;
            
            curroute = hashSGet(dn_routes, name);
            if (curroute == NULL) {
                dn_unlock(recFndLock);
                return;
            }
            
            hashSSet(weakRoutes, name, curroute);
            curWRoute = hashSGet(weakRoutes, name);
        }
        
        SF_strncpy(oldWRoute, curWRoute, DN_ROUTE_LEN);
        
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
                if (!strncmp(newRouteElements[x], routeElement[y], DN_NAME_LEN)) {
                    // It's a match, copy it in.
                    ostrlen = strlen(curWRoute);
                    SF_strncpy(curWRoute + ostrlen, routeElement[y], DN_ROUTE_LEN - ostrlen);
                    ostrlen += strlen(routeElement[y]);
                    SF_strncpy(curWRoute + ostrlen, "\n", DN_ROUTE_LEN - ostrlen);
                }
            }
        }
        
        // And put it back in the hash
        hashSSet(weakRoutes, name, curWRoute);
    }
    
    dn_unlock(recFndLock);
}

// fndPthread is the pthread that harvests late fnds
void *fndPthread(void *name_voidptr)
{
    char name[DN_NAME_LEN+1];
    char isWeak = 0;
    char *curWRoute;
    
    SF_strncpy(name, (char *) name_voidptr, DN_NAME_LEN+1);
    free(name_voidptr);
    
#ifndef __WIN32
    sleep(15);
#else
    Sleep(15000);
#endif
    
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
        hashPSet(recFndPthreads, name, (pthread_t *) -1);
        return NULL;
    }
    
    // If it's weak, send a dcr (direct connect request) (except on OSX where it doesn't work)
#ifndef __APPLE__
    {
        char *params[DN_MAX_PARAMS], *first;
        struct sockaddr locip;
        struct sockaddr_in *locip_i;
        char *rfe;
        int firfd, locip_len;
        
        memset(params, 0, DN_MAX_PARAMS * sizeof(char *));
        
        params[0] = hashSGet(dn_routes, name);
        params[1] = dn_name;
        
        // grab before the first \n for the next name in the line
        rfe = strchr(params[0], '\n');
        if (rfe == NULL) return NULL;
        first = (char *) strdup(params[0]);
        first[rfe - params[0]] = '\0';
        
        // then get the fd
        firfd = hashIGet(dn_fds, first);
        if (firfd == -1) return NULL;
        firfd = fds[firfd];
        free(first);
        // then turn the fd into a sockaddr
        locip_len = sizeof(struct sockaddr);
        if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
            locip_i = (struct sockaddr_in *) &locip;
            /*params[2] = strdup(inet_ntoa(*((struct in_addr *) &(locip_i->sin_addr))));*/
            params[2] = strdup(inet_ntoa(locip_i->sin_addr));
        } else {
            return NULL;
        }
        params[2] = realloc(params[2], strlen(params[2]) + 7);
        sprintf(params[2] + strlen(params[2]), ":%d", serv_port);
        
        handleRoutedMsg("dcr", 1, 1, params);
        
        free(params[2]);
        
        hashPSet(recFndPthreads, name, (pthread_t *) -1);
        return NULL;
    }
#endif
    return NULL;
}

/* Commands used by the UI */
int establishConnection(const char *to)
{
    char *route;
    
    route = hashSGet(dn_routes, to);
    if (route) {
        char *params[DN_MAX_PARAMS], *ip, hostbuf[128];
        struct hostent *he;
        
        memset(params, 0, DN_MAX_PARAMS * sizeof(char *));
        
        // It's a user, send a DCR
        params[1] = dn_name;
 
        gethostname(hostbuf, 128);
        he = gethostbyname(hostbuf);
        ip = inet_ntoa(*((struct in_addr *)he->h_addr));
        params[2] = (char *) malloc((strlen(ip) + 7) * sizeof(char));
        strcpy(params[2], ip);
        sprintf(params[2] + strlen(params[2]), ":%d", serv_port);
        
        handleRoutedMsg("dcr", 1, 1, params);
        
        free(params[2]);
        
        return 1;
    } else {
        // It's a hostname or IP
        return establishClient(to);
    }
}

int sendMsgB(const char *to, const char *msg, char away, char sign)
{
    char *outparams[DN_MAX_PARAMS], *route;
    char *signedmsg, *encdmsg;
    
    memset(outparams, 0, DN_MAX_PARAMS * sizeof(char *));
        
    route = hashSGet(dn_routes, to);
    if (route == NULL) {
        uiNoRoute(to);
        return 0;
    }

    outparams[0] = route;
    outparams[1] = dn_name;
    
    // Sign ...
    if (sign) {
        signedmsg = authSign(msg);
        if (!signedmsg) return 0;
    } else {
        signedmsg = strdup(msg);
    }
    
    // and encrypt the message
    encdmsg = encTo(dn_name, to, signedmsg);
    free(signedmsg);
    
    outparams[2] = encdmsg;
    if (away) {
        handleRoutedMsg("msa", 1, 1, outparams);
    } else {
        handleRoutedMsg("msg", 1, 1, outparams);
    }
        
    free(encdmsg);
    
    return 1;
}

int sendMsg(const char *to, const char *msg)
{
    return sendMsgB(to, msg, 0, 1);
}

int sendAuthKey(const char *to)
{
    char *msg;
    int ret;
    
    msg = authExport();
    if (msg) {
        ret = sendMsgB(to, msg, 0, 0);
        free(msg);
        return ret;
    } else {
        return 0;
    }
}

void sendFnd(const char *to)
{
    char outbuf[DN_CMD_LEN];
    char *toc = strdup(to);
    if (toc == NULL) { perror("strdup"); exit(1); }
    
    // Find a user by name
    buildCmd(outbuf, "fnd", 1, 1, "");
    addParam(outbuf, dn_name);
    addParam(outbuf, toc);
    addParam(outbuf, encExportKey());
        
    emitUnroutedMsg(-1, outbuf);
    
    free(toc);
}

void joinChat(const char *chat)
{
    struct hashKeyS *cur;
    
    // Add to the hash
    chatJoin(chat);
    
    // Send a cjo to everybody we have a route to
    char *outParams[4];
    
    memset(outParams, 0, 4 * sizeof(char *));
    outParams[1] = alloca(strlen(chat)+1);
    if (outParams[1] == NULL) { perror("alloca"); exit(1); }
    strcpy(outParams[1], chat);
    outParams[2] = dn_name;
    
    cur = dn_routes->head;
    while (cur) {
        outParams[0] = strdup(cur->value);
        
        handleRoutedMsg("cjo", 1, 1, outParams);
        
        free(outParams[0]);
        cur = cur->next;
    }
}

void leaveChat(const char *chat)
{
    struct hashKeyS *cur;
    
    // Remove from the hash
    chatLeave(chat);
    
    // Send a clv to everybody we have a route to
    char *outParams[4];
    
    memset(outParams, 0, 4 * sizeof(char *));
    outParams[1] = alloca(strlen(chat)+1);
    if (outParams[1] == NULL) { perror("alloca"); exit(1); }
    strcpy(outParams[1], chat);
    outParams[2] = dn_name;
    
    cur = dn_routes->head;
    while (cur) {
        outParams[0] = strdup(cur->value);
        
        handleRoutedMsg("clv", 1, 1, outParams);
        
        free(outParams[0]);
        cur = cur->next;
    }
}

void sendChat(const char *to, const char *msg)
{
    char **users;
    char *outParams[DN_MAX_PARAMS], key[DN_TRANSKEY_LEN];
    int i;
    
    memset(outParams, 0, DN_MAX_PARAMS * sizeof(char *));
    
    newTransKey(key);
    
    outParams[1] = key;
    outParams[2] = dn_name;
    outParams[3] = alloca(strlen(to)+1);
    if (outParams[3] == NULL) { perror("alloca"); exit(1); }
    strcpy(outParams[3], to);
    outParams[4] = dn_name;
    
    users = chatUsers(to);
            
    for (i = 0; users[i]; i++) {
        outParams[0] = hashSGet(dn_routes, users[i]);
        if (outParams[0] == NULL) {
            continue;
        }
        
        outParams[5] = encTo(dn_name, users[i], msg);
        
        handleRoutedMsg("cms", 1, 1, outParams);
        
        free(outParams[5]);
    }
}

void setAway(const char *msg)
{
    char *oldam;
    oldam = awayMsg;
    
    if (msg) {
        awayMsg = strdup(msg);
    } else {
        awayMsg = NULL;
    }
    
    if (oldam) {
        free(oldam);
    }
}
