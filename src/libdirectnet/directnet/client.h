/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#ifndef DN_CLIENT_H
#define DN_CLIENT_H

#include <string>

namespace DirectNet {
    /* Connect in the background to a hostname or IP
     * destination: the hostname or IP
     * requested: User has requested this connection
     */
    void async_establishClient(const std::string &destination, bool requested = true);

    /* Establish a client connection handler on an open fd. */
    void setupPeerConnection(int fd);
}

#endif // DN_CLIENT_H
