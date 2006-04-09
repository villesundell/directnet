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

#ifndef EVENT_H
#define EVENT_H 1

#ifdef __WIN32
#include <sys/timeb.h>
#endif

#include <stdlib.h>
#ifndef __WIN32
#include <sys/select.h>
#else
#include <winsock.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cassert>

#include "compat.h"

/*
 * A simple event primitive interface.
 *
 * This is designed to be easy for UIs to implement.
 */

class dn_event;

class dn_event_access; // for UI access to private members

class dn_event {
    protected:
        friend class dn_event_access;
        bool is_active;
        class dn_event_private *priv;
        dn_event() {
            is_active = false;
        }
    public:
        virtual void activate() = 0;
        virtual void deactivate() = 0;
        virtual ~dn_event() {};
        bool isActive() const { return is_active; }
        void *payload;
};

/* Timer event
 *
 * Triggers after the specified time (secs/usecs) from the epoch. Though
 * it only triggers once, it must be removed before free-ing.
 */

class dn_event_timer : public dn_event {
    protected:
        friend class dn_event_access;
        struct timeval tv;
    public:
        void (*trigger)(dn_event_timer *ev);
        struct timeval getTime() const {
            return tv;
        }
        void setTime(const struct timeval &ntv) {
            bool was_active = is_active;
            if (is_active)
                deactivate();
            tv = ntv;
            if (was_active)
                activate();
        }
        void setTimeDelta(int secs, int usecs) {
            struct timeval ntv;
            gettimeofday(&ntv, NULL);
            ntv.tv_sec += secs;
            ntv.tv_usec += usecs;

            ntv.tv_sec += ntv.tv_usec / 1000000;
            ntv.tv_usec %= 1000000;

            setTime(ntv);
        }
        virtual void activate();
        virtual void deactivate();
        dn_event_timer() : dn_event() { }
        
        dn_event_timer(
                const struct timeval &abstm,
                void (*trigger_)(dn_event_timer *ev),
                void *payload_ = NULL
                )
            : dn_event(), tv(abstm), trigger(trigger_)
        { payload = payload_; }
        
        dn_event_timer(
                int sdelta, int usdelta,
                void (*trigger_)(dn_event_timer *ev),
                void *payload_ = NULL
                )
        : dn_event(), trigger(trigger_) {
            setTimeDelta(sdelta, usdelta);
            payload = payload_;
        }

        ~dn_event_timer() {
            if (is_active)
                deactivate();
        }
};

/* fd event notification
 *
 * Triggers until removed.
 *
 * watch_cond = bitwise or of DN_EV_READ, DN_EV_WRITE, DN_EV_EXCEPT
 * cond = whichever watch_cond matched
 */
#define DN_EV_READ   1
#define DN_EV_WRITE  2
#define DN_EV_EXCEPT 4

class dn_event_fd : public dn_event {
    protected:
        friend class dn_event_access;
        int fd;
        int cond;
    public:
        void (*trigger)(dn_event_fd *ev, int cond);
        
        dn_event_fd() : dn_event() {}
        dn_event_fd(
                int fd_,
                int cond_,
                void (*trigger_)(dn_event_fd *, int),
                void *payload_ = NULL
                )
            : dn_event(), fd(fd_), cond(cond_), trigger(trigger_)
            { payload = payload_; }
        

        virtual void activate();
        virtual void deactivate();

        int getFD() const { return fd; }
        int getCond() const { return cond; }
        void setCond(int nc) {
            bool was_active = is_active;
            if (is_active)
                deactivate();
            cond = nc;
            if (was_active)
                activate();
        }

        void setFD(int nfd) {
            bool was_active = is_active;
            if (is_active)
                deactivate();
            fd = nfd;
            if (was_active)
                activate();
        }

        ~dn_event_fd() {
            if (is_active)
                deactivate();
        }
           
};

#endif
