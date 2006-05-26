/*
 * Copyright 2006  Gregor Richards
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

#ifndef DN_DHT_H
#define DN_DHT_H

#include "route.h"

// we use "routes" for our identifier lists
Route dhtlist;

/* are we fully established into this DHT?
 * ident: The identification for the DHT
 * returns: true if fully established */
bool dhtEstablished(const BinSeq &ident);

/* do a step to establish ourself
 * ident: into what DHT? */
void dhtEstablish(const BinSeq &ident);

/* what's the next hop for a search?
 * ident: what DHT?
 * key: the key being searched for
 * return: the encryption key of the next hop */

#endif
