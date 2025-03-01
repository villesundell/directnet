/*
 * Copyright 2004, 2005  Gregor Richards
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

#ifndef DN_GLOBALS_H
#define DN_GLOBALS_H

#define DN_NAME_LEN 24
#define DN_TRANSKEY_LEN 34
#define DN_MAX_CONNS 1024
#define DN_MAX_ROUTES 2048
#define DN_CMD_LEN 10240
#define DN_ROUTE_LEN 512
#define DN_MAX_PARAMS 50
#define DN_HOSTNAME_LEN 256
#define DN_KEEPALIVE_TIMER (60*5)

#define SF_strncpy(x,y,z) if (z > 0) { strncpy(x,y,z); *(x+z-1) = '\0'; }

#endif
