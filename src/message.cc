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

Message::Message(const char *scmd, char vera, char verb)
{
    cmd = scmd;
    ver[0] = vera;
    ver[1] = verb;
}

Message::Message(const string &buf)
{
    int l = buf.length(), i, s;
    
    if (l < 5) return;
    
    cmd = buf.substr(0, 3);
    ver[0] = buf[3];
    ver[1] = buf[4];
    
    for (i = 5; i < l;) {
        s = buf.find_first_of(';', i);
        if (s == string::npos) s = l;
        
        params.push_back(buf.substr(i, s - i));
        i = s + 1;
    }
}

void Message::recombineParams(int n)
{
    int i;
    
    for (i = params.size() - 2; i >= n; i--) {
        params[i] += string(";") + params[i + 1];
        params.pop_back();
    }
}
