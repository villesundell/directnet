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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dn_event.h"
#include "eventloop.h"

#undef dn_event_activate

class dn_event_private {
    public:
    int handlec;
    int handle[2];
};

gboolean evReadCallback(dn_event *ev) {
    ev->event_info.fd.trigger(DN_EV_READ, ev);
    return FALSE;
}

gboolean evWriteCallback(dn_event *ev) {
    ev->event_info.fd.trigger(DN_EV_WRITE, ev);
    return FALSE;
}

gboolean evTimeCallback(dn_event *ev) {
    ev->event_info.timer.trigger(ev);
    return FALSE;
}

void dn_event_activate(dn_event *ev) {
    dn_event_private *dep = new dn_event_private();
    ev->priv = dep;
    dep->handlec = 0;
    
    switch (ev->event_type) {
        case DN_EV_FD:
            {
                if (ev->event_info.fd.watch_cond & DN_EV_READ) {
                    dep->handle[dep->handlec] =
                        gaim_input_add(ev->event_info.fd.fd, GAIM_INPUT_READ,
                                       (GaimInputFunction) evReadCallback, ev);
                    dep->handlec++;
                }
                
                if (ev->event_info.fd.watch_cond & DN_EV_WRITE) {
                    dep->handle[dep->handlec] =
                        gaim_input_add(ev->event_info.fd.fd, GAIM_INPUT_WRITE,
                                       (GaimInputFunction) evWriteCallback, ev);
                    dep->handlec++;
                }
                return;
            }
        case DN_EV_TIMER:
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                tv.tv_sec = ev->event_info.timer.t_sec - tv.tv_sec;
                tv.tv_usec = ev->event_info.timer.t_usec - tv.tv_usec;
                tv.tv_sec += tv.tv_usec / 1000000;
                tv.tv_usec %= 1000000;
                
                dep->handlec = 1;
                dep->handle[0] =
                    gaim_timeout_add((tv.tv_sec * 1000) + (tv.tv_usec / 1000),
                                     (GSourceFunc) evTimeCallback, ev);
                return;
            }
    }
}

void dn_event_deactivate(dn_event *ev) {
    if (ev->priv == NULL) return;
    
    if (ev->event_type == DN_EV_FD) {
        int i;
        for (i = 0; i < ev->priv->handlec; i++) {
            gaim_input_remove(ev->priv->handle[i]);
        }
    } else if (ev->event_type = DN_EV_TIMER) {
        gaim_timeout_remove(ev->priv->handle[0]);
    }
    
    delete ev->priv;
    ev->priv = NULL;
}
