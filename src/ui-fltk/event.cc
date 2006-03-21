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


extern "C" {
#include "dn_event.h"
}

#include <cstdlib>
#include <FL/Fl.H>
#include <sys/time.h>
#include <ctime>
#include <map>
#include <set>
#include <cassert>
#include <iostream>
#include <cerrno>

#undef dn_event_activate

typedef struct dn_event_private {
    dn_event_t *ev;
    std::multiset<dn_event_private *>::iterator it;
    const char *trace_file;
    int trace_line;
} private_t;

static std::map<int, std::multiset<private_t *> > fd_map;

static std::set<dn_event_t *> active;

static void timer_callback(void *p) {
    private_t *priv = (private_t *)p;
    if (priv->ev) {
        if(!(priv->ev->priv == priv && priv->ev->event_type == DN_EV_TIMER)) {
            printf("assert fail, priv=%p priv->ev=%p priv->ev->priv=%p ev_type=%d (should be %d)\n", (void *)priv, (void *)priv->ev, (void *)priv->ev->priv, priv->ev->event_type, DN_EV_TIMER);
            printf("from: %s:%d\n", priv->trace_file, priv->trace_line);
            assert(false);
        }
        priv->ev->priv = NULL;
        priv->ev->event_info.timer.trigger(priv->ev);
    }
    delete priv;
}

static void fd_callback(int fd, void *unused) {
    (void)unused;
    std::multiset<dn_event_private *> &l = fd_map[fd];

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
        std::cerr << "Warning: select fail on fd " << fd << " err " << strerror(errno) << std::endl;
        return;
    }

    if (FD_ISSET(fd, &read))
        cond |= DN_EV_READ;
    if (FD_ISSET(fd, &write))
        cond |= DN_EV_WRITE;
    if (FD_ISSET(fd, &except))
        cond |= DN_EV_EXCEPT;
    
    std::multiset<dn_event_private *>::iterator it = l.begin();
    while (it != l.end()) {
        // We must do this in case the current event removes itself
        std::multiset<dn_event_private *>::iterator next = it;
        next++; // + 1 doesn't work, blah
        
        if ((*it)->ev == NULL) {
            // wtf?
            dn_event_private *p = *it;
            l.erase(it);
            delete p;
        } else {
            dn_event_t *ev = (*it)->ev;
            assert(ev->event_type == DN_EV_FD && ev->event_info.fd.fd == fd);
            interest |= ev->event_info.fd.watch_cond;
            if (ev->event_info.fd.watch_cond & cond)
                ev->event_info.fd.trigger(cond, ev);
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

void dn_event_activate(dn_event_t *event) {
#ifdef EVENT_DEBUG
    assert (active.find(event) == active.end());
    active.insert(event);
    fprintf(stderr, "Adding event %p from %s:%d, t=%d\n", (void *)event, event->trace_file, event->trace_line, event->event_type);
#endif
    switch (event->event_type) {
        case DN_EV_FD:
            {
                std::multiset<dn_event_private *> &l = fd_map[event->event_info.fd.fd];
                dn_event_private *p = new dn_event_private;
                p->ev = event;
                event->priv = p;
                p->it = l.insert(l.begin(), p);
                Fl::add_fd(
                        event->event_info.fd.fd,
                        FL_READ | FL_WRITE | FL_EXCEPT,
                        fd_callback,
                        NULL);
                p->trace_file = event->trace_file;
                p->trace_line = event->trace_line;
                break;
            }
        case DN_EV_TIMER:
            {
                event->priv = new dn_event_private;
                
#ifndef __WIN32
                struct timeval now;
                gettimeofday(&now, NULL);
#else
                struct timeb now;
                ftime(&now);
#endif
                
                double time_until = 0;
                
#ifndef __WIN32
                time_until = ((double)event->event_info.timer.t_sec - (double)now.tv_sec) +
                    ((double)event->event_info.timer.t_usec - (double)now.tv_usec) / (double)1000000;
#else
                time_until = ((double)event->event_info.timer.t_sec - (double)now.time) +
                    ((double)event->event_info.timer.t_usec - ((double)now.millitm * 1000.0)) / (double)1000000;
#endif
                
                if (time_until <= 0) {
                    event->event_info.timer.trigger(event);
                    if (event->priv) {
                        delete event->priv;
                        event->priv = NULL;
                    }
                    return;
                }
                
                event->priv->ev = event;
                
                Fl::add_timeout(time_until, timer_callback, event->priv);
                break;
            }
        default:
            assert(false);
    }
}

void dn_event_deactivate(dn_event_t *event) {
#ifdef EVENT_DEBUG
    assert (active.find(event) != active.end());
    active.erase(event);
    fprintf(stderr, "Removing event %p from %s:%d, t=%d\n", (void *)event, event->trace_file, event->trace_line, event->event_type);
#endif
    if (!event->priv)
        return;
    switch (event->event_type) {
        case DN_EV_TIMER:
            event->priv->ev = NULL;
            event->priv = NULL;
            return;
        case DN_EV_FD:
            {
                std::multiset<dn_event_private *> &l = fd_map[event->event_info.fd.fd];
                l.erase(event->priv->it);
                assert(l.find(event->priv) == l.end());
                if (l.size() == 0) {
                    Fl::remove_fd(event->event_info.fd.fd);
                }
                delete event->priv;
                event->priv = NULL;
                return;
            }
        default:
            assert(false);
    }
}
