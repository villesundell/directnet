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

#include <iostream>
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
#include "binseq.h"
#include "chat.h"
#include "client.h"
#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dn_event.h"
#include "enc.h"
#include "message.h"
#include "ui.h"

#include <set>

#define BUFSZ 65536

string *awayMsg = NULL;

void handleMsg(conn_t *conn, const BinSeq &inbuf);
bool handleNLUnroutedMsg(const Message &msg);
bool handleNRUnroutedMsg(conn_t *from, const Message &msg);
int sendMsgB(const BinSeq &to, const BinSeq &msg, bool away, bool sign);
void recvFnd(Route *route, const BinSeq &name, const BinSeq &key);

static std::set<conn_t *> active_connections;

static void send_handshake(struct connection *cs);
static void conn_notify(dn_event_fd *ev, int cond);
static void kill_connection(conn_t *conn);
static void ping_send_ev(dn_event_timer *ev);
//static void ping_timeout(dn_event_t *ev);
 
static void schedule_ping(conn_t *conn) {
    if (conn->ping_ev.isActive())
        conn->ping_ev.deactivate();
    conn->ping_ev.trigger = ping_send_ev;
    conn->ping_ev.payload = (void *)conn;
    conn->ping_ev.setTimeDelta(DN_KEEPALIVE_TIMER, 1);
    conn->ping_ev.activate();
}
 
static void ping_send_ev(dn_event_timer *ev) {
    conn_t *conn = (conn_t *) ev->payload;
    
    Message omsg(0, "pin", 1, 1);
    sendCmd(conn, omsg);
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
 
void init_comms(int fd, const string *outgh, int outgp) {
    struct connection *cs;
    assert(fd >= 0);
    cs = new connection;
    if (!cs) abort();
    cs->fd = fd;
    cs->inbuf_sz = cs->outbuf_sz = BUFSZ;
    cs->inbuf_p = cs->outbuf_p = 0;
    cs->inbuf = (unsigned char *) malloc(cs->inbuf_sz);
    cs->outbuf = (unsigned char *) malloc(cs->outbuf_sz);
    cs->fd_ev.setFD(fd);
    cs->fd_ev.setCond(DN_EV_READ | DN_EV_WRITE | DN_EV_EXCEPT);
    cs->fd_ev.trigger = conn_notify;
    cs->fd_ev.payload = (void *)cs;
    cs->state = CDN_EV_IDLE;
    cs->enckey = NULL;
    
    if (!outgh) {
        cs->outgoing = false;
        cs->outgh = NULL;
        cs->outgp = 0;
    } else {
        cs->outgoing = true;
        cs->outgh = new string(*outgh);
        cs->outgp = outgp;
    }
    
    send_handshake(cs);
}
 
void send_handshake(conn_t *cs) {
    cs->active_it = active_connections.insert(cs).first;
    
    cs->fd_ev.activate();
    
    // send DN info ...
    Message msg(0, "dni", 1, 1);
    msg.params.push_back(dn_name);
    msg.params.push_back(encExportKey());
    msg.params.push_back(BinSeq("\x00\x01\xFD\xE8", 4));
    sendCmd(cs, msg);
    
    // and hash table info
    if (cs->outgoing) {
        Message hmsg(0, "Hin", 1, 1);
        hmsg.params.push_back(dhtIn(false).toBinSeq());
        sendCmd(cs, hmsg);
    }
    
    schedule_ping(cs);
}
 
static void conn_notify_core(conn_t *conn, int cond) {
      
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
#ifndef __WIN32
        ret = recv(conn->fd, conn->inbuf + conn->inbuf_p, conn->inbuf_sz - conn->inbuf_p, 0);
#else
        ret = recv(conn->fd, (char *) (conn->inbuf + conn->inbuf_p), conn->inbuf_sz - conn->inbuf_p, 0);
#endif
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
            int i, len, sublen;
            len = 0;
            
            // get out a single message
            for (i = 6; i < conn->inbuf_p - 1; i += 2) {
                sublen = charrayToInt(((const char *) conn->inbuf) + i);
                if (sublen == 65535) {
                    // end of elements
                    len += 8; // 6 for the header + 2 for this element
                    if (conn->inbuf_p < len) {
                        // not enough
                        i = conn->inbuf_p;
                        break;
                    } else {
                        // stop here
                        break;
                    }
                } else {
                    len += sublen + 2;
                }
            }
            
            if (i >= conn->inbuf_p - 1) break;
            
            /* DEBUG
            printf("HEX: ");
            for (int j = 0; j < i; j++) {
                printf("%.2X ", conn->inbuf[j]);
            }
            printf("\nMSG: ");
            for (int j = 0; j < i; j++) {
                printf(" %c ", (conn->inbuf[j] >= 32 && conn->inbuf[j] <= 126) ?
                       conn->inbuf[j] : '.');
            }
            printf("\n\n"); */
            
            // process the message
            BinSeq buf((const char *) conn->inbuf, len);
            handleMsg(conn, buf);
            
            memmove(conn->inbuf, conn->inbuf + len, conn->inbuf_p - len);
            conn->inbuf_p -= len;
            
            if (conn->state == CDN_EV_DYING) break;
        }
    }
 
    if (cond & DN_EV_WRITE) {
        int ret;
#ifndef __WIN32
        ret = send(conn->fd, conn->outbuf, conn->outbuf_p, 0);
#else
        ret = send(conn->fd, (char *) conn->outbuf, conn->outbuf_p, 0);
#endif
        if (ret < 0) {
            perror ("send()");
            kill_connection(conn);
            return;
        }
        memmove(conn->outbuf, conn->outbuf + ret, conn->outbuf_p - ret);
        if (ret >= conn->outbuf_p) {
            conn->outbuf_p = 0;
        } else {
            conn->outbuf_p -= ret;
        }
    }
      
    conn->fd_ev.setCond(DN_EV_READ | DN_EV_EXCEPT | (conn->outbuf_p ? DN_EV_WRITE : 0));
}
 
static void kill_connection(conn_t *conn) {
    // if we have a key, we'll need to inform others
    BinSeq *key = NULL;
    if (conn->enckey) {
        key = new BinSeq(*(conn->enckey));
    }
    
    if (conn->state == CDN_EV_EVENT)
        conn->state = CDN_EV_DYING;
    else if (conn->state == CDN_EV_IDLE)
        delete conn;
    
    // inform of the disconnect
    printf("DISCONNECT:\n");
    if (key) {
        printf("OK\n");
        disNode(*key);
        delete key;
    } else {
        printf("UH OH\n");
    }
}
 
static void conn_notify(dn_event_fd *ev, int cond) {
    conn_t *conn = (conn_t *) ev->payload;
    conn->state = CDN_EV_EVENT;
    conn_notify_core(conn, cond);
    if (conn->state == CDN_EV_DYING) {
        delete conn;
    } else {
        conn->state = CDN_EV_IDLE;
    }
}
 
void destroy_conn(conn_t *conn) {
    delete conn;
}

connection::~connection() {
    if (fd_ev.isActive())
        fd_ev.deactivate();
    close(fd);
    if (enckey) {
        if (dn_names->find(*enckey) != dn_names->end()) {
            uiLoseConn((*dn_names)[*enckey]);
            dn_keys->erase((*dn_names)[*enckey]);
            dn_names->erase(*enckey);
        }
        
        if (dn_routes->find(*enckey) != dn_routes->end()) {
            delete (*dn_routes)[*enckey];
            dn_routes->erase(*enckey);
        }
        
        dn_conn->erase(*enckey);
        delete enckey;
    }
    
    if (outgh) {
        delete outgh;
    }
    
    free(inbuf);
    free(outbuf);
    
    active_connections.erase(active_it);
}

// Send a command
void sendCmd(struct connection *conn, Message &msg)
{
    int len, p;
    
    //msg.debug("OUTGOING");
    
    BinSeq buf = msg.toBinSeq();
    size_t newsz = conn->outbuf_sz;
    
    if (conn->fd < 0 || conn->state == CDN_EV_DYING)
        return;
    
    if (buf.size() >= DN_CMD_LEN) {
        // FIXME: This should inform the user
        printf("MESSAGE TOO LONG!\n");
        return;
    }
    len = buf.size();
    
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
        if (p >= len) /* all sent! */
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
    
    conn->fd_ev.setCond(conn->fd_ev.getCond() | DN_EV_WRITE);
}

#define REQ_PARAMS(x) if (msg.params.size() < x) return
#define CMD_IS(x, y, z) (msg.cmd == x && msg.ver[0] == y && msg.ver[1] == z)

void handleMsg(conn_t *conn, const BinSeq &rdbuf)
{
    int i, onparam, ostrlen;

    // Get the command itself
    Message msg(rdbuf);
    
    //msg.debug("INCOMING");
    
    /* if this is a routed message, check if we don't need to handle it */
    if (msg.type == 1 &&
        !handleRoutedMsg(msg)) return;
    
    /* let the DHT handle its commands */
    if (msg.cmd[0] == 'H') {
        handleDHTMessage(conn, msg);
        return;
    }
    
    /*****************************
     * Current protocol commands *
     *****************************/
    
    if (CMD_IS("cjo", 1, 1) ||
        CMD_IS("con", 1, 1)) {
        REQ_PARAMS(3);
        
        // "I'm on this chat"
        // Are we even on this chat?
        if (!chatOnChannel(msg.params[2].c_str())) {
            return;
        }
            
        // OK, this person is on our list for this chat
        chatAddUser(msg.params[2].c_str(), msg.params[1].c_str());
            
        // Should we echo?
        if (msg.cmd == "cjo") {
            Message nmsg(1, "con", 1, 1);
            
            if (dn_routes->find(msg.params[1]) == dn_routes->end()) {
                return;
            }
            nmsg.params.push_back((*dn_routes)[msg.params[1]]->toBinSeq());
            nmsg.params.push_back(dn_name);
            nmsg.params.push_back(msg.params[2]);
                
            handleRoutedMsg(nmsg);
        }
        
    } else if (CMD_IS("cms", 1, 1)) {
        REQ_PARAMS(6);
        
        // a chat message
        BinSeq unenmsg;
            
        // Should we just ignore it?
        if (dn_trans_keys->find(msg.params[2].c_str()) == dn_trans_keys->end()) {
            (*dn_trans_keys)[msg.params[2].c_str()] = 1;
        } else {
            return;
        }
            
        // Are we on this chat?
        if (!chatOnChannel(msg.params[3].c_str())) {
            return;
        }
            
        // Yay, chat for us!
            
        // Decrypt it
        unenmsg = encFrom(msg.params[1], dn_name, msg.params[5]);
        if (unenmsg[0] == '\0') {
            return;
        }
        uiDispChatMsg(msg.params[3].c_str(), msg.params[4].c_str(), unenmsg.c_str());
            
        // Pass it on
        if (dn_chats->find(msg.params[3].c_str()) == dn_chats->end()) {
            // explode
            return;
        }
        vector<string> *chan = (*dn_chats)[msg.params[3].c_str()];
        int s = chan->size();
            
        Message nmsg(1, "cms", 1, 1);
            
        nmsg.params.push_back("");
        nmsg.params.push_back(dn_name);
        nmsg.params.push_back(msg.params[2]);
        nmsg.params.push_back(msg.params[3]);
        nmsg.params.push_back(msg.params[4]);
        nmsg.params.push_back("");
            
        for (i = 0; i < s; i++) {
            if (dn_routes->find((*chan)[i]) == dn_routes->end()) {
                continue;
            }
            nmsg.params[0] = (*dn_routes)[(*chan)[i]]->toBinSeq();
                
            BinSeq encd = encTo(dn_name, (*chan)[i], unenmsg);
            nmsg.params[5] = encd;
                
            handleRoutedMsg(nmsg);
        }
        
    } else if (CMD_IS("dis", 1, 1)) {
        REQ_PARAMS(1);
        
        disNode(msg.params[0]);
        
    } else if (CMD_IS("dcr", 1, 1) ||
               CMD_IS("dce", 1, 1)) {
        // stub
        return;
        REQ_PARAMS(3);
        
        // This doesn't work on OSX
#if !defined(__APPLE__)
        if (CMD_IS("dcr", 1, 1)) {
            // dcr echos
            Message nmsg(1, "dce", 1, 1);
            stringstream ss;
                
            Route *route;
            BinSeq first;
            conn_t *firc;
            int firfd, locip_len, gsnr;
            struct sockaddr locip;
            struct sockaddr_in *locip_i;
                
            // First, echo, then try to connect
            if (dn_routes->find(msg.params[1]) == dn_routes->end()) return;
            route = (*dn_routes)[msg.params[1]];
            nmsg.params.push_back(route->toBinSeq());
            nmsg.params.push_back(dn_name);
                
            // grab before the first \n for the next name in the line
            first = (*route)[0];
                
            // then get the fd
            if (dn_conn->find(first) == dn_conn->end()) goto try_connect;
            
            firc = (conn_t *) (*dn_conn)[first];
            
            firfd = firc->fd;
                
            // then turn the fd into a sockaddr
            locip_len = sizeof(struct sockaddr);
#ifndef __WIN32
            gsnr = getsockname(firfd, &locip, (unsigned int *) &locip_len);
#else
            gsnr = getsockname(firfd, &locip, (int *) &locip_len);
#endif
            if (gsnr == 0) {
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
        async_establishClient(msg.params[2].c_str());
#endif

    } else if (CMD_IS("dni", 1, 1)) {
        REQ_PARAMS(3);
        
        // if the version isn't right, immediately fail
        if (msg.params[2].size() < 4) {
            kill_connection(conn);
            return;
        }
        int remver[2];
        remver[0] = charrayToInt(msg.params[2].c_str());
        if (remver[0] != 1) {
            kill_connection(conn);
            return;
        }
        remver[1] = charrayToInt(msg.params[2].c_str() + 2);
        if (remver[1] < 65000) {
            kill_connection(conn);
            return;
        }
        // FIXME: when this protocol stabilizes, this will need to set up translation
        
        // make sure they don't have the same enckey as us
        if (msg.params[1] == encExportKey()) {
            kill_connection(conn);
            return;
        }
        
        Route *route;
        BinSeq keyhash = encHashKey(msg.params[1]);
        
        // now accept the new FD
        conn->enckey = new BinSeq(msg.params[1]);
        (*dn_conn)[msg.params[1]] = conn;
        
        // and the new route
        route = new Route();
        route->push_back(keyhash);
        dn_addRoute(msg.params[1], *route);
        
        (*dn_names)[msg.params[1]] = msg.params[0];
        (*dn_keys)[msg.params[0]] = msg.params[1];
        (*dn_kbh)[keyhash] = msg.params[1];
        
        encImportKey(msg.params[0], msg.params[1]);
        
        uiEstRoute(msg.params[0].c_str());

    } else if (CMD_IS("fnd", 1, 1)) {
        conn_t *remc;
        
        REQ_PARAMS(4);
        
        if (!handleNLUnroutedMsg(msg)) {
            return;
        }
        
        Route *nroute = new Route(msg.params[0]);
        
        // Am I this person?
        if (!strcmp(msg.params[2].c_str(), dn_name)) {
            recvFnd(nroute, msg.params[1], msg.params[3]);
            delete nroute;
            return;
        }
        
        // Add myself to the route
        *nroute += pukeyhash;
        
        Message omsg(0, "fnd", 1, 1);
        omsg.params.push_back(nroute->toBinSeq());
        omsg.params.push_back(msg.params[1]);
        omsg.params.push_back(msg.params[2]);
        omsg.params.push_back(msg.params[3]);
        
        delete nroute;
        
        // Do I have a connection to this person?
        /*if (dn_conn->find(msg.params[2]) != dn_conn->end()) {
            remc = (conn_t *) (*dn_conn)[msg.params[2]];
            sendCmd(remc, omsg);
            return;
        }*/
        
        // If all else fails, continue the chain
        emitUnroutedMsg(conn, omsg);
    
    } else if (CMD_IS("fnr", 1, 1)) {
        bool handleit;
        
        REQ_PARAMS(4);
        
        handleit = handleRoutedMsg(msg);
        
        if (handleit) {
            // Hoorah, add a user
            
            // 1) Route/name
            Route *route = new Route(msg.params[2]);
            route->push_back(encHashKey(msg.params[3]));
            if (dn_routes->find(msg.params[3]) != dn_routes->end())
                delete (*dn_routes)[msg.params[3]];
            (*dn_routes)[msg.params[3]] = route;
            
            (*dn_names)[msg.params[3]] = msg.params[1];
            (*dn_keys)[msg.params[1]] = msg.params[3];
            (*dn_kbh)[encHashKey(msg.params[3])] = msg.params[3];
            
            // 2) Key
            encImportKey(msg.params[1], msg.params[3]);
            
            uiEstRoute(msg.params[1]);
        }
    
    } else if (CMD_IS("lst", 1, 1)) {
        REQ_PARAMS(2);
        
        uiLoseRoute(msg.params[1].c_str());
        
    } else if (CMD_IS("msg", 1, 1) ||
               CMD_IS("msa", 1, 1)) {
        REQ_PARAMS(3);
        
        // This is our message
        BinSeq unencmsg, *dispmsg;
        char *sig, *sigm;
        int austat, iskey;
            
        // Decrypt it
        unencmsg = encFrom(msg.params[1], dn_name, msg.params[2]);
            
        // And verify the signature
        dispmsg = authVerify(unencmsg, &sig, &austat);
            
        // Make our signature tag
        iskey = 0;
        if (austat == -1) {
            sigm = strdup("n");
        } else if (austat == 0) {
            sigm = strdup("u");
        } else if (austat == 1 && sig) {
            sigm = (char *) malloc((strlen(sig) + 3) * sizeof(char));
            sprintf(sigm, "s %s", sig);
        } else if (austat == 1 && !sig) {
            sigm = strdup("s");
        } else if (austat == 2) {
            /* this IS a signature, totally different response */
            uiAskAuthImport(msg.params[1], unencmsg, sig);
            iskey = 1;
            sigm = NULL;
        } else {
            sigm = strdup("?");
        }
        if (sig) free(sig);
            
        if (!iskey) {
            uiDispMsg(msg.params[1].c_str(), dispmsg->c_str(), sigm, msg.cmd == "msa");
            free(sigm);
        }
        delete dispmsg;
            
        // Are we away?
        if (awayMsg && msg.cmd != "msa") {
            sendMsgB(msg.params[1].c_str(), *awayMsg, true, true);
        }
            
        return;
        
        
    } else if (CMD_IS("pir", 1, 1)) {
        REQ_PARAMS(3);
        
        // just pong
        Message rmsg(1, "por", 1, 1);
        rmsg.params.push_back(msg.params[2]);
        rmsg.params.push_back(dn_name);
        handleRoutedMsg(rmsg);
        
        
    }
}

bool handleRoutedMsg(const Message &msg)
{
    int i, s;
    conn_t *sendc;
    Route *route;
    BinSeq next, buf, last;
    
    if (msg.params[0].size() <= 1) {
        return true; // This is our data
    }
    
    route = new Route(msg.params[0]);
    
    if (route->size() < 1) {
        return true; // broken - presume our data
    }
    
    // see everyone in the route
    seeUsers((*route));
    
    next = (*route)[0];
    route->pop_front();
    
    if (dn_kbh->find(next) == dn_kbh->end()) {
        // failure
        return true;
    }
    next = (*dn_kbh)[next];
    
    if (dn_conn->find(next) == dn_conn->end()) {
        delete route;
        return 0;
    }
    
    sendc = (conn_t *) (*dn_conn)[next];
    
    Message omsg(msg.type, msg.cmd.c_str(), msg.ver[0], msg.ver[1]);
    omsg.params.push_back(route->toBinSeq());
    s = msg.params.size();
    for (i = 1; i < s; i++) {
        omsg.params.push_back(msg.params[i]);
    }
    
    sendCmd(sendc, omsg);
    
    delete route;
    return 0;
}

bool handleNLUnroutedMsg(const Message &msg)
{
    Route *route = new Route(msg.params[0]);
    int i, s;
    
    s = route->size();
    for (i = 0; i < s; i++) {
        if ((*route)[i] == encExportKey()) {
            delete route;
            return false;
        }
    }
    return true;
}

bool handleNRUnroutedMsg(conn_t *from, const Message &msg) {
    // This part of handleUnroutedMsg decides whether to just delete it
    if (dn_trans_keys->find(msg.params[0].c_str()) == dn_trans_keys->end()) {
        int i, s;
        
        (*dn_trans_keys)[msg.params[0].c_str()] = 1;
        
        // Continue the transmission
        Message omsg(msg.type, msg.cmd.c_str(), msg.ver[0], msg.ver[1]);
        s = msg.params.size();
        for (i = 0; i < s; i++) {
            omsg.params.push_back(msg.params[i]);
        }
        emitUnroutedMsg(from, omsg);
        
        return true;
    }
    return false;
}

void emitUnroutedMsg(conn_t *from, Message &outbuf)
{
    std::set<conn_t *>::iterator it = active_connections.begin();
    
    while (it != active_connections.end()) {
        if (*it != from)
            sendCmd(*it, outbuf);
        it++;
    }
}

// recvFnd handles the situation that we've just been "found"
void recvFnd(Route *route, const BinSeq &name, const BinSeq &key)
{
    // Ignore it if we already have a route
    if (dn_routes->find(key) != dn_routes->end()) {
        return;
    }
    BinSeq keyhash = encHashKey(key);
    
    // Add his route,
    (*dn_routes)[key] = new Route(*route);
    
    (*dn_names)[key] = name;
    (*dn_keys)[name] = key;
    (*dn_kbh)[keyhash] = key;
    
    // and public key,
    encImportKey(name, key);
    
    // then send your route back to him
    /*Message omsg(2, "fnr", 1, 1);
    omsg.params.push_back(reverseRoute->toBinSeq());
    omsg.params.push_back(dn_name);
    omsg.params.push_back(route->toBinSeq());
    omsg.params.push_back(encExportKey());
    handleRoutedMsg(omsg);*/
    
    uiEstRoute(name.c_str());
    
    /* Send a dcr (direct connect request) (except on OSX where it doesn't work)
#ifndef __APPLE__
    {
        Message nmsg(1, "dcr", 1, 1);
        stringstream ss;
        
        Route *route;
        string first;
        conn_t *firc;
        int firfd, locip_len;
        struct sockaddr locip;
        struct sockaddr_in *locip_i;
        
        // First, echo, then try to connect
        if (dn_routes->find(key) == dn_routes->end()) return;
        route = (*dn_routes)[key];
        nmsg.params.push_back(route->toBinSeq());
        nmsg.params.push_back(dn_name);
        
        // grab before the first \n for the next name in the line
        first = (*route)[0];
        
        // then get the fd
        if (dn_conn->find(first) == dn_conn->end()) return;
        
        firc = (conn_t *) (*dn_conn)[first];
        
        firfd = firc->fd;
        
        // then turn the fd into a sockaddr
        locip_len = sizeof(struct sockaddr);
#ifndef __WIN32
        int gsnr = getsockname(firfd, &locip, (unsigned int *) &locip_len);
#else
        int gsnr = getsockname(firfd, &locip, (int *) &locip_len);
#endif
        if (gsnr == 0) {
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
#endif*/
}

/* Commands used by the UI */
void establishConnection(const string &to)
{
    if (dn_keys->find(to) != dn_keys->end() &&
        dn_routes->find((*dn_keys)[to]) != dn_routes->end()) {
        Route *route = (*dn_routes)[(*dn_keys)[to]];
        char hostbuf[128], *ip;
        struct hostent *he;
        stringstream ss;
        
        // it's a user, send a dcr
        Message omsg(1, "dcr", 1, 1);
        omsg.params.push_back(route->toBinSeq());
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

int sendMsgB(const BinSeq &to, const BinSeq &msg, bool away, bool sign)
{
    Message omsg(1, "msg", 1, 1);
    Route *route;
    BinSeq encdmsg, *signedmsg;
    
    if (dn_keys->find(to) == dn_keys->end() ||
        dn_routes->find((*dn_keys)[to]) == dn_routes->end()) {
        uiNoRoute(to);
        return 0;
    }
    route = (*dn_routes)[(*dn_keys)[to]];
    
    omsg.params.push_back(route->toBinSeq());
    omsg.params.push_back(dn_name);
    
    // Sign ...
    if (sign) {
        signedmsg = authSign(msg);
        if (!signedmsg) return 0;
    } else {
        signedmsg = new BinSeq(msg);
        if (!signedmsg) { perror("strdup"); exit(1); }
    }
    
    // and encrypt the message
    encdmsg = encTo(dn_name, to, *signedmsg);
    delete signedmsg;
    
    omsg.params.push_back(encdmsg);
    
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
    BinSeq *msg;
    int ret;
    
    msg = authExport();
    if (msg) {
        ret = sendMsgB(to, *msg, false, false);
        delete msg;
        return ret;
    } else {
        return 0;
    }
}

void sendFnd(const string &toc) {
    // Find a user by name
    Message omsg(0, "fnd", 1, 1);
    omsg.params.push_back("\xFF\xFF");
    omsg.params.push_back(dn_name);
    omsg.params.push_back(toc);
    omsg.params.push_back(encExportKey());
    
    emitUnroutedMsg(NULL, omsg);
}

void joinChat(const string &chat)
{
    // Join the chat
    chatJoin(chat);
    
    // Send a cjo to everybody we have a route to
    Message msg(1, "cjo", 1, 1);
    msg.params.push_back("");
    msg.params.push_back(dn_name);
    msg.params.push_back(chat);
    
    map<BinSeq, Route *>::iterator ri;
    
    for (ri = dn_routes->begin(); ri != dn_routes->end(); ri++) {
        msg.params[0] = ri->second->toBinSeq();
        handleRoutedMsg(msg);
    }
}

void leaveChat(const string &chat)
{
    // Leave the chat
    chatLeave(chat);
    
    // Send a clv to everybody we have a route to
    Message msg(1, "clv", 1, 1);
    msg.params.push_back("");
    msg.params.push_back(dn_name);
    msg.params.push_back(chat);
    
    map<BinSeq, Route *>::iterator ri;
    
    for (ri = dn_routes->begin(); ri != dn_routes->end(); ri++) {
        msg.params[0] = ri->second->toBinSeq();
        handleRoutedMsg(msg);
    }
}

void sendChat(const string &to, const string &msg)
{
    char key[DN_TRANSKEY_LEN];
    vector<string> *chan;
    int i, s;
    
    Message omsg(1, "cms", 1, 1);
    omsg.params.push_back("");
    omsg.params.push_back(dn_name);
    
    newTransKey(key);
    omsg.params.push_back(key);
    
    omsg.params.push_back(to);
    omsg.params.push_back(dn_name);
    omsg.params.push_back("");
    
    
    if (dn_chats->find(to) == dn_chats->end()) {
        return;
    }
    chan = (*dn_chats)[to];
    s = chan->size();
    
    for (i = 0; i < s; i++) {
        if (dn_keys->find((*chan)[i]) == dn_keys->end() ||
            dn_routes->find((*dn_keys)[(*chan)[i]]) == dn_routes->end())
            continue;
        
        omsg.params[0] = (*dn_routes)[(*dn_keys)[(*chan)[i]]]->toBinSeq();
        omsg.params[5] = encTo(dn_name, (*chan)[i], msg);
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

void disNode(const BinSeq &key)
{
    BinSeq hkey = encHashKey(key);
    
    if (dn_seen_user->find(hkey) == dn_seen_user->end()) {
        // I don't know this user, so I don't care
        return;
    }
    
    // remove from the seen_user list
    dn_seen_user->erase(hkey);
    
    // remove any routes leading to or through this user (complicated)
    map<BinSeq, Route *>::iterator dri;
    for (dri = dn_routes->begin(); dri != dn_routes->end(); dri++) {
        if (dri->second &&
            dri->second->find(hkey)) {
            // try to get a new one
            /* FIXME: DHT FIND USER */
            
            // bad route, delete it
            delete dri->second;
            dn_routes->erase(dri);
            //dri--;
        }
    }
    
    // forward a disconnect message
    Message msg(0, "dis", 1, 1);
    msg.params.push_back(key);
    
    emitUnroutedMsg(NULL, msg);
    
    // DHT sanity checks (we may have just broken our DHT links!)
    dhtCheck();
}
