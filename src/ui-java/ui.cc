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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "client.h"
#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dnconfig.h"
#include "enc.h"
#include "ui.h"
#include "dn_event.h"

#include <assert.h>

#include <sys/time.h>
#include <event.h>

extern "C" int _call_java(int, int, int, int);

extern "C" void javaSetNick(char *to)
{
    strncpy(dn_name, to, DN_NAME_LEN);
}

int main(int argc, char **argv, char **envp)
{
    // This UI just sends info into Java
    event_init();
    
    if (findEnc(envp) == -1) {
        printf("Necessary encryption programs were not found on your PATH!\n");
        return -1;
    }
    
    authSetPW("", "");
    
    // Then ask for the nick
    _call_java(1, 0, 0, 0); // java syscall 1 = set nick, use javaSetNick
    
    // And creating the key
    encCreateKey();
    
    dn_goOnline();
    
    event_loop(0);
    return 0;
}

void uiDispMsg(const string &from, const string &msg, const string &authmsg, int away)
{
    // java syscall 2 = display message
    struct {
        const char *from;
        const char *msg;
        const char *authmsg;
        int away;
    } minfo;
    minfo.from = from.c_str();
    minfo.msg = msg.c_str();
    minfo.authmsg = authmsg.c_str();
    minfo.away = away;
    _call_java(2, (int) &minfo, 0, 0);
}

extern "C" void javaAuthImport(char *key)
{
    authImport(key);
}

void uiAskAuthImport(const string &from, const string &msg, const string &sig)
{
    // java syscall 8 = ask to authorize import of authentication key
    _call_java(8, (int) from.c_str(), (int) msg.c_str(), (int) sig.c_str());
    // java code should call javaAuthImport if yes
}

void uiDispChatMsg(const string &chat, const string &from, const string &msg)
{
    // java syscall 3 = display a chat message
    _call_java(3, (int) chat.c_str(), (int) from.c_str(), (int) msg.c_str());
}

void uiEstConn(const string &from)
{
    // java syscall 4 = established connection
    _call_java(4, (int) from.c_str(), 0, 0);
}

void uiEstRoute(const string &from)
{
    // java syscall 5 = established route
    _call_java(5, (int) from.c_str(), 0, 0);
}

void uiLoseConn(const string &from)
{
    // java syscall 6 = lost connection
    _call_java(6, (int) from.c_str(), 0, 0);
}

void uiLoseRoute(const string &from)
{
    // java syscall 7 = lost route
    _call_java(7, (int) from.c_str(), 0, 0);
}

void uiNoRoute(const string &to)
{
    // java syscall 9 = no route
    _call_java(9, (int) to.c_str(), 0, 0);
}

// things to let Java access DirectNet:
extern "C" {
    void javaSetAway(char *msg)
    {
        if (msg) {
            string smsg = msg;
            setAway(&smsg);
        } else {
            setAway(NULL);
        }
    }

    void javaEstablishConnection(char *host)
    {
        establishConnection(host);
    }

    void javaSendFnd(char *user)
    {
        sendFnd(user);
    }

    void javaSendMsg(char *user, char *msg)
    {
        sendMsg(user, msg);
    }

    void javaSendChat(char *room, char *msg)
    {
        sendChat(room, msg);
    }
}

class dn_event_private {
    public:
    struct event cevt;
    dn_event *parent;
};

class dn_event_access {
    public:

        static void callback(int fd, short cond, void *payload) {
            dn_event *ev = (dn_event *) payload;
            dn_event_fd *fde = dynamic_cast<dn_event_fd *>(ev);
            dn_event_timer *timer = dynamic_cast<dn_event_timer *>(ev);

            if (fde) {
                int c = 0;
                if (cond & EV_READ)
                    c |= DN_EV_READ;
                if (cond & EV_WRITE)
                    c |= DN_EV_WRITE;
                fde->trigger(fde, c);
            } else if (timer) {
                /* libevent clears the event for us in this case, clean up
                 * the private data
                 */
                delete timer->priv;
                timer->priv = NULL;
                timer->trigger(timer);
            } else assert(0 == "bad event type");
        }
};

void dn_event_fd::activate() {

    assert(!is_active);
    
    priv = new dn_event_private();
    priv->parent = this;

    int cond = EV_PERSIST;
    if (this->cond & DN_EV_READ)
        cond |= EV_READ;
    if (this->cond & DN_EV_WRITE)
        cond |= EV_WRITE;
    event_set(&priv->cevt, this->fd, cond, dn_event_access::callback, static_cast<dn_event *>(this));
    event_add(&priv->cevt, NULL);

    is_active = true;
}

void dn_event_timer::activate() {
    assert(!is_active);

    priv = new dn_event_private();
    priv->parent = this;
        
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tv.tv_sec = this->tv.tv_sec - tv.tv_sec;
    tv.tv_usec = this->tv.tv_usec - tv.tv_usec;
    tv.tv_sec += tv.tv_usec / 1000000;
    tv.tv_usec %= 1000000;
    evtimer_set(&priv->cevt, dn_event_access::callback, static_cast<dn_event *>(this));
    event_add(&priv->cevt, &tv);
    
    is_active = true;
}

void dn_event_fd::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    event_del(&priv->cevt);
    delete priv;
    priv = NULL;
}

void dn_event_timer::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    event_del(&priv->cevt);
    delete priv;
    priv = NULL;
}
