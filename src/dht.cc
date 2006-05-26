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

#include <string.h>

#include "dht.h"

map<BinSeq, DHTInfo> in_dhts;

DHTInfo::DHTInfo()
{
    rep = NULL;
    neighbors[0] = NULL;
    neighbors[1] = NULL;
    neighbors[2] = NULL;
    neighbors[3] = NULL;
    established = false;
}

void dhtJoin(const BinSeq &ident, const BinSeq &rep)
{
    in_dhts.erase(ident);
    in_dhts[ident];
}

bool dhtEstablished(const BinSeq &ident)
{
    if (in_dhts.find(ident) == in_dhts.end()) return false;
    return in_dhts[ident].established;
}

void dhtEstablish(const BinSeq &ident)
{
    // TODO: implement
}

BinSeq dhtNextHop(const BinSeq &ident, const BinSeq &key)
{
    // TODO: implement
}
