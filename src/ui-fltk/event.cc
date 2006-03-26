/*
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
 */


#include "dn_event.h"

#include <cstdlib>
#include <FL/Fl.H>
#include <sys/time.h>
#include <ctime>
#include <map>
#include <set>
#include <cassert>
#include <iostream>
#include <cerrno>
using namespace std;

struct dn_event_private {
    dn_event *ev;
    multiset<dn_event_private *>::iterator it;
};

static map<int, multiset<dn_event_private *> > fd_map;

static set<dn_event *> active;

class dn_event_access {
    public:
        static void timer_callback(void *p) {
            dn_event_private *priv = (dn_event_private *)p;
            if (priv->ev) {
                dn_event_timer *t = dynamic_cast<dn_event_timer *>(priv->ev);
                assert(t);
                assert(t->priv == priv);
                t->priv = NULL;
                t->trigger(t);
            }
            delete priv;
        }
        static void fd_callback(int fd, void *unused) {
            (void)unused;
            multiset<dn_event_private *> &l = fd_map[fd];

            if (l.size() == 0) {
                Fl::remove_fd(fd);
                fd_map.erase(fd);
                return;
            }

            int cond = 0;
            int interest = 0;
            fd_set read, write, except;
            FD_ZERO(&read);
            FD_SET(fd, &read);
            FD_ZERO(&write);
            FD_SET(fd, &write);
            FD_ZERO(&except);
            FD_SET(fd, &except);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            int ret = select(fd + 1, &read, &write, &except, &timeout);
            if (ret == -1) {
                cerr << "Warning: select fail on fd " << fd << " err " << strerror(errno) << endl;
                return;
            }

            if (FD_ISSET(fd, &read))
                cond |= DN_EV_READ;
            if (FD_ISSET(fd, &write))
                cond |= DN_EV_WRITE;
            if (FD_ISSET(fd, &except))
                cond |= DN_EV_EXCEPT;
            
            multiset<dn_event_private *>::iterator it = l.begin();
            while (it != l.end()) {
                // We must do this in case the current event removes itself
                multiset<dn_event_private *>::iterator next = it;
                next++; // + 1 doesn't work, blah
                
                if ((*it)->ev == NULL) {
                    // wtf?
                    dn_event_private *p = *it;
                    l.erase(it);
                    delete p;
                } else {
                    dn_event_fd *ev = dynamic_cast<dn_event_fd *>((*it)->ev);
                    assert(ev);
                    assert(ev->getFD() == fd);
        //            assert(ev->priv == (*it));
                    interest |= ev->cond;
                    if (ev->getCond() & cond)
                        ev->trigger(ev, cond);
                }
                it = next;
            }
            Fl::remove_fd(fd);
            
            int new_watch = 0;
            if (interest & DN_EV_READ)
                new_watch |= FL_READ;
            if (interest & DN_EV_WRITE)
                new_watch |= FL_WRITE;
            if (interest & DN_EV_EXCEPT)
                new_watch |= FL_EXCEPT;
            
            if (l.size() && new_watch)
                Fl::add_fd(fd, new_watch, fd_callback, NULL);
        }
};

void dn_event_fd::activate() {
    assert(!is_active);
    
    multiset<dn_event_private *> &l = fd_map[fd];
    dn_event_private *p = new dn_event_private;
//    std::cerr << "dn_ev_fd act this=" << (void *)this << " p=" << (void *)p << std::endl;
    p->ev = this;
    priv = p;
    p->it = l.insert(l.begin(), p);
    Fl::add_fd(
            fd,
            FL_READ | FL_WRITE | FL_EXCEPT,
            dn_event_access::fd_callback,
            NULL);
    is_active = true;
}

void dn_event_timer::activate() {
    assert(!is_active);
    priv = new dn_event_private;
//    std::cerr << "dn_ev_timer act this=" << (void *)this << " p=" << (void *)priv << std::endl;
    struct timeval now;
    gettimeofday(&now, NULL);
                
    double time_until = 0;
                
    time_until = (double)tv.tv_sec - (double)now.tv_sec;
    time_until += ((double)tv.tv_usec - (double)now.tv_usec) / (double)1000000;
                
    priv->ev = this;
                
    Fl::add_timeout(time_until, dn_event_access::timer_callback, (void *)priv);
    is_active = true;
}

void dn_event_timer::deactivate() {
    if (!is_active)
        return;
//    std::cerr << "timer deact " << (void *)this << std::endl;
    if (priv)
        priv->ev = NULL;
    priv = NULL;
    is_active = false;
}

void dn_event_fd::deactivate() {
    if (!is_active) return;
    multiset<dn_event_private *> &l = fd_map[fd];
//    std::cerr << "fd deact " << (void *)this << std::endl;
    l.erase(priv->it);
    assert(l.find(priv) == l.end());
    if (l.size() == 0) {
        Fl::remove_fd(fd);
    }
    delete priv;
    priv = NULL;
    is_active = false;
}
