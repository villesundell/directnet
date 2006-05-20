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

#include "message.h"

Message::Message(char stype, const char *scmd, char vera, char verb)
{
    type = stype;
    cmd = scmd;
    ver[0] = vera;
    ver[1] = verb;
}

Message::Message(const BinSeq &buf)
{
    int l = buf.size(), i, j, pl;
    BinSeq *push;
    vector<int> lens;
    
    if (l < 8) return;
    
    type = buf[0];
    cmd = buf.substr(1, 3);
    ver[0] = buf[4];
    ver[1] = buf[5];
    
    for (i = 6; i < l - 1; i += 2) {
        pl = charrayToInt(&(buf.front()) + i);
        if (pl == 65535) break;
        lens.push_back(pl);
    }
    
    for (i += 2, j = 0; i < l && j < lens.size(); j++) {
        params.push_back(buf.substr(i, lens[j]));
        i += lens[j];
    }
}

BinSeq Message::toBinSeq()
{
    BinSeq toret;
    int i;
    char c[2];
    
    toret += type;
    toret += cmd;
    toret.push_back(ver, 2);
    
    for (i = 0; i < params.size(); i++) {
        intToCharray(params[i].size(), c);
        toret.push_back(c, 2);
    }
    toret.push_back("\xFF\xFF", 2);
    
    for (i = 0; i < params.size(); i++) {
        toret.push_back(params[i]);
    }
    
    return toret;
}

int charrayToInt(const char *c)
{
    int i = 0;
    i = (unsigned char) c[0];
    i <<= 8;
    i += ((unsigned char) c[1]);
    return i;
}

void intToCharray(int i, char *c)
{
    c[0] = i >> 8;
    c[1] = i & 0xFF;
}
