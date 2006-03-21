/*
 * Copyright 2004, 2005, 2006  Gregor Richards
 * Copyright 2006 Bryan Donlan
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
#include <assert.h>

#include "auth.h"
#include "chat.h"
#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "dn_event.h"
#include "enc.h"
#include "hash.h"
#include "ui.h"

#define BUFSZ 65536

char *awayMsg = NULL;

void handleMsg(conn_t *conn, const char *inbuf);
int handleNLUnroutedMsg(char **params);
int handleNRUnroutedMsg(conn_t *from, const char *command, char vera, char verb, char **params);
int sendMsgB(const char *to, const char *msg, char away, char sign);
void recvFnd(char *route, const char *name, const char *key);

enum cev_state {
    CDN_EV_IDLE, CDN_EV_EVENT, CDN_EV_DYING
};

struct connection {
    conn_t *prev, *next;
    int fd;
    enum cev_state state;
      
    unsigned char *inbuf, *outbuf;
    size_t inbuf_sz, outbuf_sz;
    size_t inbuf_p, outbuf_p;
      
    char *name;
 
    dn_event_t ev, *ping_ev;
};
 
static conn_t ring_head = {
    &ring_head, &ring_head,
    -1,
    CDN_EV_DYING
    /* ignore missing initializer warnings here */
};
 
static void send_handshake(struct connection *cs);
static void conn_notify(int cond, dn_event_t *ev);
static void kill_connection(conn_t *conn);
static void destroy_conn(conn_t *conn);
static void ping_send_ev(dn_event_t *ev);
//static void ping_timeout(dn_event_t *ev);
 
static void schedule_ping(conn_t *conn) {
    assert(!conn->ping_ev);
    conn->ping_ev = dn_event_trigger_after(ping_send_ev, conn, DN_KEEPALIVE_TIMER, 1);
}
 
static void ping_send_ev(dn_event_t *ev) {
    conn_t *conn = (conn_t *) ev->payload;
    assert(conn->ping_ev == ev);
    dn_event_deactivate(ev);
    free(ev);
    conn->ping_ev = NULL;
 
    sendCmd(conn, "pin\x01\x01;");
    //    conn->ping_ev = dn_event_trigger_after(ping_timeout, conn, 30, 0);
    schedule_ping(conn);
}
 
#if 0
static void ping_timeout(dn_event_t *ev) {
    conn_t *conn = ev->payload;
    assert(conn->ping_ev == ev);
    dn_event_deactivate(ev);
    free(ev);
    conn->ping_ev = NULL;
 
    fprintf("ping timeout on conn %p, fd %d\n", (void *)conn, conn->fd);
    kill_connection(conn);
}
#endif
 
void init_comms(int fd) {
    struct connection *cs;
    assert(fd >= 0);
    cs = (struct connection *) malloc(sizeof *cs);
    if (!cs) abort();
    cs->fd = fd;
    cs->inbuf_sz = cs->outbuf_sz = BUFSZ;
    cs->inbuf_p = cs->outbuf_p = 0;
    cs->inbuf = (unsigned char *) malloc(cs->inbuf_sz);
    cs->outbuf = (unsigned char *) malloc(cs->outbuf_sz);
    cs->ev.payload = cs;
    cs->ev.event_type = DN_EV_FD;
    cs->ev.event_info.fd.fd = fd;
    cs->ev.event_info.fd.watch_cond = DN_EV_READ | DN_EV_WRITE | DN_EV_EXCEPT;
    cs->ev.event_info.fd.trigger = conn_notify;
    cs->state = CDN_EV_IDLE;
    cs->name = NULL;
    cs->ping_ev = NULL;
      
    send_handshake(cs);
}
 
void send_handshake(conn_t *cs) {
    char buf[DN_CMD_LEN];
 
    cs->prev = &ring_head;
    cs->next = ring_head.next;
    cs->next->prev = cs;
    cs->prev->next = cs;
 
    dn_event_activate(&cs->ev);
    buildCmd(buf, "key", 1, 1, dn_name);
    addParam(buf, encExportKey());
    sendCmd(cs, buf);
 
    schedule_ping(cs);
}
 
static void conn_notify_core(int cond, conn_t *conn) {
      
    if (cond & DN_EV_EXCEPT) {
        // disconnect
        kill_connection(conn);
        return;
    }
 
    if (cond & DN_EV_READ) {
        int ret;
        if (conn->inbuf_p == conn->inbuf_sz) {
            // buffer is full, break the connection
            kill_connection(conn);
            return;
        }
        ret = recv(conn->fd, conn->inbuf + conn->inbuf_p, conn->inbuf_sz - conn->inbuf_p, 0);
        if (ret == 0) {
            // connection closed
            kill_connection(conn);
            return;
        }
        if (ret < 0) {
            perror("recv()");
            kill_connection(conn);
            return;
        }
        conn->inbuf_p += ret;
        while (conn->inbuf_p) {
            int i;
            for (i = 0; i < conn->inbuf_p; i++)
                if (!conn->inbuf[i]) break;
            if (conn->inbuf_p == i) break;
            // \0 in msg, process it
            handleMsg(conn, (char *) conn->inbuf);
            memmove(conn->inbuf, conn->inbuf + i + 1, conn->inbuf_p - i - 1);
            conn->inbuf_p -= i + 1;
        }
    }
 
    if (cond & DN_EV_WRITE) {
        int ret;
        ret = send(conn->fd, conn->outbuf, conn->outbuf_p, 0);
        if (ret < 0) {
            perror ("send()");
            kill_connection(conn);
            return;
        }
        memmove(conn->outbuf, conn->outbuf + ret, conn->outbuf_p - ret);
        conn->outbuf_p -= ret;
    }
      
    dn_event_deactivate(&conn->ev);
    conn->ev.event_info.fd.watch_cond = DN_EV_READ | DN_EV_EXCEPT | (conn->outbuf_p ? DN_EV_WRITE : 0);
    dn_event_activate(&conn->ev);
}
 
static void kill_connection(conn_t *conn) {
    if (conn->state == CDN_EV_EVENT)
        conn->state = CDN_EV_DYING;
    else if (conn->state == CDN_EV_IDLE)
        destroy_conn(conn);
}
 
static void conn_notify(int cond, dn_event_t *ev) {
    conn_t *conn = (conn_t *) ev->payload;
    conn->state = CDN_EV_EVENT;
    conn_notify_core(cond, conn);
    if (conn->state == CDN_EV_DYING) {
        destroy_conn(conn);
    } else {
        conn->state = CDN_EV_IDLE;
    }
}
 
void destroy_conn(conn_t *conn) {
    close(conn->fd);
    if (conn->name) {
        uiLoseConn(conn->name);
        hashSDelKey(dn_routes, conn->name);
    }
    dn_event_deactivate(&conn->ev);
    if (conn->ping_ev) {
        dn_event_deactivate(conn->ping_ev);
        free(conn->ping_ev);
    }
         
    free(conn->name);
    free(conn->inbuf);
    free(conn->outbuf);
 
    conn->next->prev = conn->prev;
    conn->prev->next = conn->next;
      
    free(conn);
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
void addParam(char *into, const char *newparam)
{
    /*sprintf(into+strlen(into), ";%s", newparam);*/
    into[DN_CMD_LEN - 2] = '\0';
    int cpied = strlen(into);
    
    into[cpied] = ';';
    cpied++;
    
    strncpy(into + cpied, newparam, DN_CMD_LEN - cpied);
}

// Send a command
void sendCmd(struct connection *conn, const char *buf)
{
    int len, p;
    size_t newsz = conn->outbuf_sz;

    if (conn->fd < 0 || conn->state == CDN_EV_DYING)
        return;
    
    len = strlen(buf) + 1;
    p = 0;
    if (conn->outbuf_p == 0) {
        // try an immediate send, as an optimization
        p = send(conn->fd, buf, len, 0);
        if (p < 0) {
            /* Whoops, something broke.
             * Let the event handler clean up.
             */
            p = 0;
        }
        if (p == len) /* all sent! */
            return;
    }

    while (conn->outbuf_p + len - p > newsz)
        newsz += 16384;
    if (newsz != conn->outbuf_sz) {
        fprintf(stderr, "WARNING: Output buffer full for conn %p (%zu < %zu), expanding to %zu\n", (void *)conn, conn->outbuf_sz, conn->outbuf_p + len - p, newsz);
        conn->outbuf = (unsigned char *) realloc(conn->outbuf, newsz);
        if (!conn->outbuf) abort();
        conn->outbuf_sz = newsz;
    }

    memcpy(conn->outbuf + conn->outbuf_p, buf + p, len - p);
    conn->outbuf_p += len - p;

    dn_event_deactivate(&conn->ev);
    conn->ev.event_info.fd.watch_cond |= DN_EV_WRITE;
    dn_event_activate(&conn->ev);
}

static void reap_fnd_later(const char *name);

#define REQ_PARAMS(x) if (params[x-1] == NULL) return
#define REJOIN_PARAMS(x) { \
	int i; \
	for (i = x; params[i] != NULL; i++) { \
		*(params[i]-1) = ';'; \
		params[i] = NULL; \
	} \
}
			
void handleMsg(conn_t *conn, const char *rdbuf)
{
    char command[4], *params[DN_MAX_PARAMS];
    char *inbuf;
    int i, onparam, ostrlen;

    // Get the command itself
    inbuf = (char *) alloca(strlen(rdbuf)+1);
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
            if (hashIGet(dn_trans_keys, params[1]) == -1) {
                hashISet(dn_trans_keys, params[1], 1);
            } else {
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
#if 1 && !defined(__APPLE__)
            if (!strncmp(command, "dcr", 3) &&
                inbuf[3] == 1 && inbuf[4] == 1) {
                // dcr echos
                char *outParams[DN_MAX_PARAMS];
                char *rfe, *first;
                conn_t *firc;
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
                if (rfe == NULL) goto try_connect;
                first = (char *) strdup(outParams[0]);
                first[rfe - outParams[0]] = '\0';
                // then get the fd
                firc = (conn_t *) hashVGet(dn_conn, first);
                free(first);
                if (!firc) goto try_connect;
                firfd = firc->fd;
                // then turn the fd into a sockaddr
                locip_len = sizeof(struct sockaddr);
                if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
                    locip_i = (struct sockaddr_in *) &locip;
                    /*params[2] = strdup(inet_ntoa(*((struct in_addr *) &(locip_i->sin_addr))));*/
                    outParams[2] = strdup(inet_ntoa(locip_i->sin_addr));
                } else {
                    goto try_connect;
                }
                outParams[2] = (char *) realloc(outParams[2], strlen(outParams[2]) + 7);
                sprintf(outParams[2] + strlen(outParams[2]), ":%d", serv_port);
                
                handleRoutedMsg("dce", 1, 1, outParams);
                
                free(outParams[2]);
            }
try_connect:
            
            // Then, attempt the connection
            async_establishClient(params[2]);
#endif
        }

    } else if (!strncmp(command, "fnd", 3) &&
        inbuf[3] == 1 && inbuf[4] == 1) {
        conn_t *remc;
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
        remc = (conn_t *) hashVGet(dn_conn, params[2]);
        if (remc) {
            sendCmd(remc, outbuf);
            return;
        }
        
        // If all else fails, continue the chain
        emitUnroutedMsg(conn, outbuf);
    
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
        hashVSet(dn_conn, params[0], conn);
        conn->name = strdup(params[0]);
        if (!conn->name) abort();
        
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
    }
}

int handleRoutedMsg(const char *command, char vera, char verb, char **params)
{
    int i;
    conn_t *sendc;
    char *route, *newroute, newbuf[DN_CMD_LEN];
    
    if (strlen(params[0]) == 0) {
        return 1; // This is our data
    }
    
    route = strdup(params[0]);
    
    for (i = 0; route[i] != '\n' && route[i] != '\0'; i++);
    route[i] = '\0';
    newroute = route+i+1;
    
    sendc = (conn_t *) hashVGet(dn_conn, route);
        
    if (!sendc) {
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
    
    sendCmd(sendc, newbuf);
    
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

int handleNRUnroutedMsg(conn_t *from, const char *command, char vera, char verb, char **params) {
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
        emitUnroutedMsg(from, outbuf);
        
        return 1;
    }
    return 0;
}

void emitUnroutedMsg(conn_t *from, const char *outbuf)
{
    // This emits an unrouted message
    conn_t *p;
    if (!from)
        from = &ring_head;
    p = from->next;

    while (p != from) {
        sendCmd(p, outbuf);
        p = p->next;
    }
}

// recvFnd handles the situation that we've just been "found"
void recvFnd(char *route, const char *name, const char *key)
{
    char *routeElement[DN_MAX_PARAMS], *outparams[5];
    char reverseRoute[DN_ROUTE_LEN];
    int onRE, i, ostrlen;
    char newroute[DN_ROUTE_LEN];
    
    memset(routeElement, 0, DN_MAX_PARAMS * sizeof(char *));
    memset(outparams, 0, 5 * sizeof(char *));
    
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
                if (onRE >= DN_MAX_PARAMS - 1) onRE--;
            }
        }
    }
    routeElement[onRE] = NULL;

    // FIXME: needs event hash
            
    // If we haven't started one, good
    if (1) {
    // This is the first fnd received, but ignore it if we already have a route
        if (hashSGet(dn_routes, name)) {
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
        
        reap_fnd_later(name);
        
        // Put it back where it belongs
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
                    if (onRE >= DN_MAX_PARAMS - 1) onRE--;
                }
            }
        }
        newRouteElements[onRE] = NULL;
        
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
    
}

static void fnd_reap(dn_event_t *ev);

static void reap_fnd_later(const char *name_p) {
    char *name;
    if (!name_p) abort();
    name = strdup(name_p);
    if (!name) abort();
    dn_event_trigger_after(fnd_reap, name, 15, 0);
}
    

static void fnd_reap(dn_event_t *ev)
{
    char *name = (char *) ev->payload;
    char isWeak = 0;
    char *curWRoute;
    
    dn_event_deactivate(ev);
    free(ev);
    
    
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
        //hashPSet(recFndPthreads, name, (pthread_t *) -1);
        return;
    }
    
    // If it's weak, send a dcr (direct connect request) (except on OSX where it doesn't work)
#ifndef __APPLE__
    {
        char *params[DN_MAX_PARAMS], *first;
        struct sockaddr locip;
        struct sockaddr_in *locip_i;
        char *rfe;
        int firfd, locip_len;
        conn_t *firc;
        
        memset(params, 0, DN_MAX_PARAMS * sizeof(char *));
        
        params[0] = hashSGet(dn_routes, name);
        params[1] = dn_name;
        
        // grab before the first \n for the next name in the line
        rfe = strchr(params[0], '\n');
        if (rfe == NULL) {
            return;
        }
        first = (char *) strdup(params[0]);
        first[rfe - params[0]] = '\0';
        
        // then get the fd
        firc = (conn_t *) hashVGet(dn_conn, first);
        free(first);
        if (!firc) {
            return;
        }
        firfd = firc->fd;
        // then turn the fd into a sockaddr
        locip_len = sizeof(struct sockaddr);
        if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
            locip_i = (struct sockaddr_in *) &locip;
            /*params[2] = strdup(inet_ntoa(*((struct in_addr *) &(locip_i->sin_addr))));*/
            params[2] = strdup(inet_ntoa(locip_i->sin_addr));
        } else {
            return;
        }
        params[2] = (char *) realloc(params[2], strlen(params[2]) + 7);
        sprintf(params[2] + strlen(params[2]), ":%d", serv_port);
        
        handleRoutedMsg("dcr", 1, 1, params);
        
        free(params[2]);
        
//        hashPSet(recFndPthreads, name, (pthread_t *) -1);
        return;
    }
#endif
    return;
}

/* Commands used by the UI */
void establishConnection(const char *to)
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
    } else {
        // It's a hostname or IP
        async_establishClient(to);
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

void sendFnd(const char *toc) {
    char outbuf[DN_CMD_LEN];
    
    // Find a user by name
    buildCmd(outbuf, "fnd", 1, 1, "");
    addParam(outbuf, dn_name);
    addParam(outbuf, toc);
    addParam(outbuf, encExportKey());
        
    emitUnroutedMsg(NULL, outbuf);
}

void joinChat(const char *chat)
{
    struct hashKeyS *cur;
    
    // Add to the hash
    chatJoin(chat);
    
    // Send a cjo to everybody we have a route to
    char *outParams[4];
    
    memset(outParams, 0, 4 * sizeof(char *));
    outParams[1] = (char *) alloca(strlen(chat)+1);
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
    outParams[1] = (char *) alloca(strlen(chat)+1);
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
    outParams[3] = (char *) alloca(strlen(to)+1);
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
