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

#include <string>
#include <sstream>
using namespace std;

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
#include "message.h"
#include "ui.h"

#define BUFSZ 65536

string *awayMsg = NULL;

void handleMsg(conn_t *conn, const char *inbuf);
bool handleNLUnroutedMsg(const Message &msg);
bool handleNRUnroutedMsg(conn_t *from, const Message &msg);
int sendMsgB(const string &to, const string &msg, bool away, bool sign);
void recvFnd(Route *route, const string &name, const string &key);

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
    
    string cmd = "pin\x01\x01;";
    sendCmd(conn, cmd);
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
    string buf;
 
    cs->prev = &ring_head;
    cs->next = ring_head.next;
    cs->next->prev = cs;
    cs->prev->next = cs;
 
    dn_event_activate(&cs->ev);
    buf = buildCmd("key", 1, 1, dn_name);
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
        if (dn_routes->find(conn->name) != dn_routes->end()) {
            delete (*dn_routes)[conn->name];
            dn_routes->erase(conn->name);
        }
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
string buildCmd(const string &command, char vera, char verb, const string &param)
{
    stringstream toret;
    toret << command << vera << verb << param;
    return toret.str();
}

// Add a parameter into a command buffer
void addParam(string &into, const string &newparam)
{
    into += string(";") + newparam;
}

// Send a command
void sendCmd(struct connection *conn, string &buf)
{
    int len, p;
    size_t newsz = conn->outbuf_sz;

    if (conn->fd < 0 || conn->state == CDN_EV_DYING)
        return;
    
    if (buf.size() >= DN_CMD_LEN) {
        buf = buf.substr(0, DN_CMD_LEN - 1);
    }
    len = buf.length() + 1;
    p = 0;
    if (conn->outbuf_p == 0) {
        // try an immediate send, as an optimization
        p = send(conn->fd, buf.c_str(), len, 0);
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

    memcpy(conn->outbuf + conn->outbuf_p, buf.c_str() + p, len - p);
    conn->outbuf_p += len - p;

    dn_event_deactivate(&conn->ev);
    conn->ev.event_info.fd.watch_cond |= DN_EV_WRITE;
    dn_event_activate(&conn->ev);
}

static void reap_fnd_later(const char *name);

#define REQ_PARAMS(x) if (msg.params.size() < x) return
#define REJOIN_PARAMS(x) msg.recombineParams((x)-1)
			
void handleMsg(conn_t *conn, const char *rdbuf)
{
    int i, onparam, ostrlen;

    // Get the command itself
    Message msg(rdbuf);
    
    /*****************************
     * Current protocol commands *
     *****************************/
    
    if ((msg.cmd == "cjo" &&
         msg.ver[0] == 1 && msg.ver[1] == 1) ||
        (msg.cmd == "con" &&
         msg.ver[0] == 1 && msg.ver[1] == 1)) {
        REQ_PARAMS(3);
        
        // "I'm on this chat"
        if (handleRoutedMsg(msg)) {
            // Are we even on this chat?
            if (!chatOnChannel(msg.params[1])) {
                return;
            }
            
            // OK, this person is on our list for this chat
            chatAddUser(msg.params[1], msg.params[2]);
            
            // Should we echo?
            if (msg.cmd == "cjo") {
                Message nmsg("con", 1, 1);
                
                if (dn_routes->find(msg.params[2]) == dn_routes->end()) {
                    return;
                }
                nmsg.params.push_back((*dn_routes)[msg.params[2]]->toString());
                nmsg.params.push_back(msg.params[1]);
                nmsg.params.push_back(dn_name);
                
                handleRoutedMsg(nmsg);
            }
        }
        
    } else if ((msg.cmd == "cms" &&
                msg.ver[0] == 1 && msg.ver[1] == 1)) {
        REQ_PARAMS(6);
        
        // a chat message
        if (handleRoutedMsg(msg)) {
            char *unenmsg;
            
            // Should we just ignore it?
            if (dn_trans_keys->find(msg.params[1]) == dn_trans_keys->end()) {
                (*dn_trans_keys)[msg.params[1]] = 1;
            } else {
                return;
            }
            
            // Are we on this chat?
            if (!chatOnChannel(msg.params[3])) {
                return;
            }
            
            // Yay, chat for us!
            
            // Decrypt it
            unenmsg = encFrom(msg.params[2].c_str(), dn_name, msg.params[5].c_str());
            if (unenmsg[0] == '\0') {
                free(unenmsg);
                return;
            }
            // STRINGIFY:
            uiDispChatMsg(msg.params[3].c_str(), msg.params[4].c_str(), unenmsg);
            
            // Pass it on
            if (dn_chats->find(msg.params[3]) == dn_chats->end()) {
                // explode
                free(unenmsg);
                return;
            }
            vector<string> *chan = (*dn_chats)[msg.params[3]];
            int s = chan->size();
            
            Message nmsg("cms", 1, 1);
            
            nmsg.params.push_back("");
            nmsg.params.push_back(msg.params[1]);
            nmsg.params.push_back(dn_name);
            nmsg.params.push_back(msg.params[3]);
            nmsg.params.push_back(msg.params[4]);
            nmsg.params.push_back("");
            
            for (i = 0; i < s; i++) {
                if (dn_routes->find((*chan)[i]) == dn_routes->end()) {
                    continue;
                }
                nmsg.params[0] = (*dn_routes)[(*chan)[i]]->toString();
                
                char *encd = encTo(dn_name, (*chan)[i].c_str(), unenmsg);
                nmsg.params[5] = encd;
                free(encd);
                
                handleRoutedMsg(nmsg);
            }
            
            free(unenmsg);
        }
        
    } else if ((msg.cmd == "dcr" &&
                msg.ver[0] == 1 && msg.ver[1] == 1) ||
               (msg.cmd == "dce" &&
                msg.ver[0] == 1 && msg.ver[1] == 1)) {
        REQ_PARAMS(3);
        
        if (handleRoutedMsg(msg)) {
            // This doesn't work on OSX
#if !defined(__APPLE__)
            if (msg.cmd == "dcr" &&
                msg.ver[0] == 1 && msg.ver[1] == 1) {
                // dcr echos
                Message nmsg("dce", 1, 1);
                stringstream ss;
                
                Route *route;
                string first;
                conn_t *firc;
                int firfd, locip_len;
                struct sockaddr locip;
                struct sockaddr_in *locip_i;
                
                // First, echo, then try to connect
                if (dn_routes->find(msg.params[1]) == dn_routes->end()) return;
                route = (*dn_routes)[msg.params[1]];
                nmsg.params.push_back(route->toString());
                nmsg.params.push_back(dn_name);
                
                // grab before the first \n for the next name in the line
                first = (*route)[0];
                
                // then get the fd
                if (dn_conn->find(first) == dn_conn->end()) goto try_connect;
                
                firc = (conn_t *) (*dn_conn)[first];
                
                firfd = firc->fd;
                
                // then turn the fd into a sockaddr
                locip_len = sizeof(struct sockaddr);
                if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
                    locip_i = (struct sockaddr_in *) &locip;
                    ss.str("");
                    ss << inet_ntoa(locip_i->sin_addr);
                } else {
                    goto try_connect;
                }
                ss << string(":") << serv_port;
                nmsg.params.push_back(ss.str());
                
                handleRoutedMsg(nmsg);
                
                try_connect:
                1;
            }
            
            // Then, attempt the connection
            async_establishClient(msg.params[2]);
#endif
        }

    } else if (msg.cmd == "fnd" &&
               msg.ver[0] == 1 && msg.ver[1] == 1) {
        conn_t *remc;
        string outbuf;
        
        REQ_PARAMS(4);
        
        if (!handleNLUnroutedMsg(msg)) {
            return;
        }
        
        Route *nroute = new Route(msg.params[0]);
        
        // Am I this person?
        if (msg.params[2] == dn_name) {
            recvFnd(nroute, msg.params[1], msg.params[3]);
            delete nroute;
            return;
        }
        
        // Add myself to the route
        *nroute += dn_name;
        
        outbuf = buildCmd("fnd", 1, 1, nroute->toString());
        addParam(outbuf, msg.params[1]);
        addParam(outbuf, msg.params[2]);
        addParam(outbuf, msg.params[3]);
        
        delete nroute;
        
        // Do I have a connection to this person?
        if (dn_conn->find(msg.params[2]) != dn_conn->end()) {
            remc = (conn_t *) (*dn_conn)[msg.params[2]];
            sendCmd(remc, outbuf);
            return;
        }
        
        // If all else fails, continue the chain
        emitUnroutedMsg(conn, outbuf);
    
    } else if (msg.cmd == "fnr" &&
               msg.ver[0] == 1 && msg.ver[1] == 1) {
        bool handleit;
        
        REQ_PARAMS(4);
        
        handleit = handleRoutedMsg(msg);
        
        if (!handleit) {
            // This isn't our route, but do add intermediate routes
            Route *ra, *rb;
            string endu;
            
            // Who's the end user?
            ra = new Route(msg.params[0]);
            if (ra->size() > 0) {
                endu = (*ra)[ra->size()-1];
                
                // And add an intermediate route
                (*dn_iRoutes)[endu] = ra;
            }
            
            // Then figure out the route from here back
            rb = new Route(msg.params[2]);
            
            // Loop through to find my name
            while (rb->size() > 0 && (*rb)[0] != dn_name) rb->pop_front();
            
            // And add that route
            if (rb->size() > 0)
                (*dn_iRoutes)[msg.params[1]] = rb;
        } else {
            // Hoorah, add a user
            
            // 1) Route
            Route *route = new Route(msg.params[2]);
            if (dn_routes->find(msg.params[1]) != dn_routes->end())
                delete (*dn_routes)[msg.params[1]];
            (*dn_routes)[msg.params[1]] = route;
            if (dn_iRoutes->find(msg.params[1]) != dn_iRoutes->end())
                delete (*dn_iRoutes)[msg.params[1]];
            (*dn_iRoutes)[msg.params[1]] = new Route(*route);
            
            // 2) Key
            encImportKey(msg.params[1].c_str(), msg.params[3].c_str());
            
            // STRINGIFY:
            uiEstRoute(msg.params[1].c_str());
        }
    
    } else if (msg.cmd == "key" &&
               msg.ver[0] == 1 && msg.ver[1] == 1) {
        REQ_PARAMS(2);
        
        Route *route;
        
        // if I already have a route to this person, drop it
        if (dn_routes->find(msg.params[0]) != dn_routes->end()) {
            delete (*dn_routes)[msg.params[0]];
            dn_routes->erase(msg.params[0]);
        }
        
        // now accept the new FD
        conn->name = strdup(msg.params[0].c_str());
        if (!conn->name) abort();
        (*dn_conn)[msg.params[0]] = conn;
        
        route = new Route(msg.params[0] + "\n");
        if (dn_routes->find(msg.params[0]) != dn_routes->end())
            delete (*dn_routes)[msg.params[0]];
        (*dn_routes)[msg.params[0]] = route;
        if (dn_iRoutes->find(msg.params[0]) != dn_iRoutes->end())
            delete (*dn_iRoutes)[msg.params[0]];
        (*dn_iRoutes)[msg.params[0]] = new Route(*route);
        
        encImportKey(msg.params[0].c_str(), msg.params[1].c_str());
        
        // STRINGIFY:
        uiEstRoute(msg.params[0].c_str());

    } else if (msg.cmd == "lst" &&
               msg.ver[0] == 1 && msg.ver[1] == 1) {
        REQ_PARAMS(2);
      
        // STRINGIFY:
        uiLoseRoute(msg.params[1].c_str());
    
    } else if ((msg.cmd == "msg" &&
                msg.ver[0] == 1 && msg.ver[1] == 1) ||
               (msg.cmd == "msa" &&
                msg.ver[0] == 1 && msg.ver[1] == 1)) {
        REQ_PARAMS(3);
	REJOIN_PARAMS(3);
        
        if (handleRoutedMsg(msg)) {
            // This is our message
            char *unencmsg, *dispmsg, *sig, *sigm;
            int austat, iskey;
            
            // Decrypt it
            unencmsg = encFrom(msg.params[1].c_str(), dn_name, msg.params[2].c_str());
            
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
                // STRINGIFY:
                uiAskAuthImport(msg.params[1].c_str(), unencmsg, sig);
                iskey = 1;
                sigm = NULL;
            } else {
                free(unencmsg);
                sigm = strdup("?");
            }
            if (sig) free(sig);
            
            if (!iskey) {
                // STRINGIFY:
                uiDispMsg(msg.params[1].c_str(), dispmsg, sigm, msg.cmd == "msa");
                free(sigm);
            }
            free(dispmsg);
            
            // Are we away?
            if (awayMsg && msg.cmd != "msa") {
                sendMsgB(msg.params[1], *awayMsg, true, true);
            }
            
            return;
        }
    }
}

bool handleRoutedMsg(const Message &msg)
{
    int i, s;
    conn_t *sendc;
    Route *route;
    string next, buf, last;
    
    if (msg.params[0].length() == 0) {
        return true; // This is our data
    }
    
    route = new Route(msg.params[0]);
    
    next = (*route)[0];
    route->pop_front();
    
    if (dn_conn->find(next) == dn_conn->end()) {
        // We need to tell the other side that this route is dead!
        // Bouncing a lost could end up in an infinite loop, so don't
        if (msg.cmd != "lst") {
            string endu;
            
            route->push_front(next);
            endu = (*route)[route->size()-1];
            
            // If this is me, don't send the command, just display it
            if (msg.params[1] == dn_name) {
                // STRINGIFY
                uiLoseRoute(endu.c_str());
            } else {
                Message lstm("lst", 1, 1);
                Route *iroute;
                
                // Do we have an intermediate route?
                if (dn_iRoutes->find(msg.params[1]) == dn_iRoutes->end()) return 0;
                iroute = (*dn_iRoutes)[msg.params[1]];
                
                // Send the command
                lstm.params.push_back(iroute->toString());
                lstm.params.push_back(endu);
                handleRoutedMsg(lstm);
            }
        }
        
        delete route;
        return 0;
    }
    
    sendc = (conn_t *) (*dn_conn)[next];
    
    buf = buildCmd(msg.cmd, msg.ver[0], msg.ver[1], route->toString());
    s = msg.params.size();
    for (i = 1; i < s; i++) {
        addParam(buf, msg.params[i]);
    }
    
    sendCmd(sendc, buf);
    
    delete route;
    return 0;
}

bool handleNLUnroutedMsg(const Message &msg)
{
    Route *route = new Route(msg.params[0]);
    int i, s;
    
    s = route->size();
    for (i = 0; i < s; i++) {
        if ((*route)[i] == dn_name) {
            delete route;
            return false;
        }
    }
    return true;
}

bool handleNRUnroutedMsg(conn_t *from, const Message &msg) {
    // This part of handleUnroutedMsg decides whether to just delete it
    if (dn_trans_keys->find(msg.params[0]) == dn_trans_keys->end()) {
        string outbuf;
        int i, s;
        
        (*dn_trans_keys)[msg.params[0]] = 1;
        
        // Continue the transmission
        outbuf = buildCmd(msg.cmd, msg.ver[0], msg.ver[1], msg.params[0]);
        s = msg.params.size();
        for (i = 1; i < s; i++) {
            addParam(outbuf, msg.params[i]);
        }
        emitUnroutedMsg(from, outbuf);
        
        return true;
    }
    return false;
}

void emitUnroutedMsg(conn_t *from, string &outbuf)
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
void recvFnd(Route *route, const string &name, const string &key)
{
    // FIXME: needs event hash
    
    // If we haven't started one, good
    if (1) {
        // This is the first fnd received, but ignore it if we already have a route
        if (dn_routes->find(name) != dn_routes->end()) {
            return;
        }
                
        // If we haven't already gotten the fastest route, this must be it
        // Build backwards route
        Route *reverseRoute = new Route(*route);
        reverseRoute->reverse();
        
        // Add his route,
        if (dn_routes->find(name) != dn_routes->end())
            delete (*dn_routes)[name];
        (*dn_routes)[name] = new Route(*route);
        if (dn_iRoutes->find(name) != dn_iRoutes->end())
            delete (*dn_iRoutes)[name];
        (*dn_iRoutes)[name] = new Route(*route);
        
        // and public key,
        encImportKey(name.c_str(), key.c_str());
        
        // then send your route back to him
        Message omsg("fnr", 1, 1);
        omsg.params.push_back(reverseRoute->toString());
        omsg.params.push_back(dn_name);
        omsg.params.push_back(route->toString());
        omsg.params.push_back(encExportKey());
        handleRoutedMsg(omsg);
        
        // STRINGIFY:
        uiEstRoute(name.c_str());
        
        reap_fnd_later(name.c_str());
        
        // Put it back where it belongs
    } else {
#if 0
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
#endif
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
    string sname = string(name);
    
    dn_event_deactivate(ev);
    free(ev);
    
    // Send a dcr (direct connect request) (except on OSX where it doesn't work)
#ifndef __APPLE__
    {
        Message nmsg("dcr", 1, 1);
        stringstream ss;
        
        Route *route;
        string first;
        conn_t *firc;
        int firfd, locip_len;
        struct sockaddr locip;
        struct sockaddr_in *locip_i;
        
        // First, echo, then try to connect
        if (dn_routes->find(sname) == dn_routes->end()) return;
        route = (*dn_routes)[sname];
        nmsg.params.push_back(route->toString());
        nmsg.params.push_back(dn_name);
        
        // grab before the first \n for the next name in the line
        first = (*route)[0];
        
        // then get the fd
        if (dn_conn->find(first) == dn_conn->end()) return;
        
        firc = (conn_t *) (*dn_conn)[first];
        
        firfd = firc->fd;
        
        // then turn the fd into a sockaddr
        locip_len = sizeof(struct sockaddr);
        if (getsockname(firfd, &locip, (unsigned int *) &locip_len) == 0) {
            locip_i = (struct sockaddr_in *) &locip;
            ss.str("");
            ss << inet_ntoa(locip_i->sin_addr);
        } else {
            return;
        }
        ss << string(":") << serv_port;
        nmsg.params.push_back(ss.str());
        
        handleRoutedMsg(nmsg);
    }
#endif
}

/* Commands used by the UI */
void establishConnection(const string &to)
{
    string sto = string(to);
    
    if (dn_routes->find(sto) != dn_routes->end()) {
        Route *route = (*dn_routes)[sto];
        char hostbuf[128], *ip;
        struct hostent *he;
        stringstream ss;
        
        // it's a user, send a dcr
        Message omsg("dcr", 1, 1);
        omsg.params.push_back(route->toString());
        omsg.params.push_back(dn_name);
        
        hostbuf[127] = '\0';
        gethostname(hostbuf, 127);
        he = gethostbyname(hostbuf);
        ip = inet_ntoa(*((struct in_addr *)he->h_addr));
        
        ss.str("");
        ss << ip << ":" << serv_port;
        omsg.params.push_back(ss.str());
        
        handleRoutedMsg(omsg);
    } else {
        // It's a hostname or IP
        async_establishClient(to);
    }
}

#include <iostream>
using namespace std;

int sendMsgB(const string &to, const string &msg, bool away, bool sign)
{
    Message omsg("msg", 1, 1);
    Route *route;
    char *signedmsg, *encdmsg;
    
    if (dn_routes->find(to) == dn_routes->end()) {
        // STRINGIFY:
        uiNoRoute(to.c_str());
        return 0;
    }
    route = (*dn_routes)[to];
    
    omsg.params.push_back(route->toString());
    omsg.params.push_back(dn_name);
    
    // Sign ...
    if (sign) {
        signedmsg = authSign(msg.c_str());
        if (!signedmsg) return 0;
    } else {
        signedmsg = strdup(msg.c_str());
        if (!signedmsg) { perror("strdup"); exit(1); }
    }
    
    // and encrypt the message
    encdmsg = encTo(dn_name, to.c_str(), signedmsg);
    free(signedmsg);
    
    omsg.params.push_back(encdmsg);
    free(encdmsg);
    
    if (away) {
        omsg.cmd = "msa";
    }
    
    handleRoutedMsg(omsg);
    
    return 1;
}

int sendMsg(const string &to, const string &msg)
{
    return sendMsgB(to, msg, false, true);
}

int sendAuthKey(const string &to)
{
    char *msg;
    int ret;
    
    msg = authExport();
    if (msg) {
        ret = sendMsgB(to, msg, false, false);
        free(msg);
        return ret;
    } else {
        return 0;
    }
}

void sendFnd(const string &toc) {
    string outbuf;
    
    // Find a user by name
    outbuf = buildCmd("fnd", 1, 1, "");
    addParam(outbuf, dn_name);
    addParam(outbuf, toc);
    addParam(outbuf, encExportKey());
    
    emitUnroutedMsg(NULL, outbuf);
}

void joinChat(const string &chat)
{
    // Join the chat
    chatJoin(chat);
    
    // Send a cjo to everybody we have a route to
    Message msg("cjo", 1, 1);
    msg.params.push_back("");
    msg.params.push_back(chat);
    msg.params.push_back(dn_name);
    
    map<string, Route *>::iterator ri;
    
    for (ri = dn_routes->begin(); ri != dn_routes->end(); ri++) {
        msg.params[0] = ri->second->toString();
        handleRoutedMsg(msg);
    }
}

void leaveChat(const string &chat)
{
    // Leave the chat
    chatLeave(chat);
    
    // Send a clv to everybody we have a route to
    Message msg("clv", 1, 1);
    msg.params.push_back("");
    msg.params.push_back(chat);
    msg.params.push_back(dn_name);
    
    map<string, Route *>::iterator ri;
    
    for (ri = dn_routes->begin(); ri != dn_routes->end(); ri++) {
        msg.params[0] = ri->second->toString();
        handleRoutedMsg(msg);
    }
}

void sendChat(const string &to, const string &msg)
{
    char key[DN_TRANSKEY_LEN];
    vector<string> *chan;
    int i, s;
    
    Message omsg("cms", 1, 1);
    omsg.params.push_back("");
    
    newTransKey(key);
    omsg.params.push_back(key);
    
    omsg.params.push_back(dn_name);
    omsg.params.push_back(to);
    omsg.params.push_back(dn_name);
    omsg.params.push_back("");
    
    
    if (dn_chats->find(to) == dn_chats->end()) {
        return;
    }
    chan = (*dn_chats)[to];
    s = chan->size();
    
    for (i = 0; i < s; i++) {
        if (dn_routes->find((*chan)[i]) == dn_routes->end()) continue;
        omsg.params[0] = (*chan)[i];
        omsg.params[5] = encTo(dn_name, (*chan)[i].c_str(), msg.c_str());
        handleRoutedMsg(omsg);
    }
}

void setAway(const string *msg)
{
    if (msg) {
        if (!awayMsg) awayMsg = new string();
        *awayMsg = *msg;
    } else {
        if (awayMsg) delete awayMsg;
        awayMsg = NULL;
    }
}
