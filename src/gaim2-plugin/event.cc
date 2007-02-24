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
    dn_event *parent;
    int handlec;
    int handle[2];
};

gboolean evReadCallback(dn_event_fd *ev) {
    ev->trigger(ev, DN_EV_READ);
    return FALSE;
}

gboolean evWriteCallback(dn_event_fd *ev) {
    ev->trigger(ev, DN_EV_WRITE);
    return FALSE;
}

gboolean evTimeCallback(dn_event_timer *ev) {
    ev->trigger(ev);
    return FALSE;
}

void dn_event_fd::activate() {
    assert(!is_active);
    
    priv = new dn_event_private();
    priv->parent = this;
    
    priv->handlec = 0;
    
    if (cond & DN_EV_READ) {
        priv->handle[priv->handlec] =
            gaim_input_add(fd, GAIM_INPUT_READ,
                           (GaimInputFunction) evReadCallback, this);
        priv->handlec++;
    }
                
    if (cond & DN_EV_WRITE) {
        priv->handle[priv->handlec] =
            gaim_input_add(fd, GAIM_INPUT_WRITE,
                           (GaimInputFunction) evWriteCallback, this);
        priv->handlec++;
    }
    return;
    
    is_active = true;
}

void dn_event_timer::activate() {
    assert(!is_active);

    priv = new dn_event_private();
    priv->parent = this;
    
    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    tv2.tv_sec = tv.tv_sec - tv2.tv_sec;
    tv2.tv_usec = tv.tv_usec - tv2.tv_usec;
    tv2.tv_sec += tv2.tv_usec / 1000000;
    tv2.tv_usec %= 1000000;
    
    priv->handlec = 1;
    priv->handle[0] =
        gaim_timeout_add((tv2.tv_sec * 1000) + (tv2.tv_usec / 1000),
                         (GSourceFunc) evTimeCallback, this);
    
    is_active = true;
}

void dn_event_fd::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    
    int i;
    for (i = 0; i < priv->handlec; i++) {
        gaim_input_remove(priv->handle[i]);
    }
    
    delete priv;
    priv = NULL;
}

void dn_event_timer::deactivate() {
    assert(is_active);
    is_active = false;
    if (priv == NULL) return;
    
    gaim_timeout_remove(priv->handle[0]);
    
    delete priv;
    priv = NULL;
}
