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

/*
 * A simple event primitive interface.
 *
 * This is designed to be easy for UIs to implement.
 */

class dn_event_private;

class dn_event;

/* Timer event
 *
 * Triggers after the specified time (secs/usecs) from the epoch. Though
 * it only triggers once, it must be removed before free-ing.
 */

#define DN_EV_TIMER 0x100
class event_info_timer {
    public:
    void (*trigger)(dn_event *ev);
    unsigned long t_sec, t_usec;
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

#define DN_EV_FD 0x101
class event_info_fd {
    public:
    void (*trigger)(int cond, dn_event *ev);
    int fd;
    char watch_cond;
};

class dn_event {
    public:
    dn_event(void *spayload, int stype, const char *stfile, int stline) {
        payload = spayload;
        event_type = stype;
        trace_file = stfile;
        trace_line = stline;
    }
    
    void *payload;
    /* Fields below this point must not be modified when the event is
     * active
     */
    int event_type;
    union {
        event_info_timer timer;
        event_info_fd fd;
    } event_info;
    dn_event_private *priv;
    const char *trace_file;
    int trace_line;
};

void dn_event_activate(dn_event *event);
void dn_event_deactivate(dn_event *event);

/* Convenience... */
static inline dn_event *dn_event_trigger_after_(void (*callback)(dn_event *), void *payload, unsigned long secs, unsigned long usecs, const char *f, int l) {
    dn_event *ev;
    
#ifndef __WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
#else
    struct timeb tp;
    ftime(&tp);
#endif
    
    ev = new dn_event(payload, DN_EV_TIMER, f, l);
    ev->event_info.timer.trigger = callback;
    
#ifndef __WIN32
    ev->event_info.timer.t_sec = tv.tv_sec + secs;
    ev->event_info.timer.t_usec = tv.tv_usec + usecs;
#else
    ev->event_info.timer.t_sec = tp.time + secs;
    ev->event_info.timer.t_usec = ((int) tp.millitm * 1000) + usecs;
#endif
    
    ev->event_info.timer.t_sec += ev->event_info.timer.t_usec / 1000000;
    ev->event_info.timer.t_usec %= 1000000;
    dn_event_activate(ev);
    return ev;
}
#define dn_event_trigger_after(c, p, s, u) dn_event_trigger_after_(c, p, s, u, __FILE__, __LINE__)

static inline dn_event *event_fd_watch_(void (*callback)(int cond, dn_event *ev), void *payload, int cond, int fd, const char *f, int l) {
    dn_event *ev = new dn_event(payload, DN_EV_FD, f, l);
    ev->event_info.fd.fd = fd;
    ev->event_info.fd.watch_cond = cond;
    ev->event_info.fd.trigger = callback;
    dn_event_activate(ev);
    return ev;
}
#define dn_event_fd_watch(c, p, cond, fd) event_fd_watch_(c, p, cond, fd, __FILE__, __LINE__)

inline static void ev_ac(dn_event *event) { dn_event_activate(event); }
#define dn_event_activate(e) {   \
    (e)->trace_file = __FILE__;  \
    (e)->trace_line = __LINE__;  \
    ev_ac(e);                    \
}

#endif
