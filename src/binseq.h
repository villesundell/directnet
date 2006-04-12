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

#ifndef DN_BINSEQ_H
#define DN_BINSEQ_H

#include <string>
#include <vector>
using namespace std;

#include <string.h>

class BinSeq : vector<unsigned char> {
    public:
    BinSeq();
    BinSeq(const BinSeq &copy);
    BinSeq(const string &copy);
    BinSeq(const unsigned char *copy, int len);
    
    // string-like functionality:
    inline unsigned char *c_str() { return &front(); }
    int find(const unsigned char* str, int index, int length);
    inline int find(unsigned char ch, int index)
    {
        return find(&ch, index, 1);
    }
    inline int find(const unsigned char* str, int index)
    {
        return find(str, index, strlen(str));
    }
    inline int find(const string& str, int index)
    {
        return find(str.c_str(), index);
    }
    inline int find(const BinSeq& str, int index)
    {
        return find(str.c_str(), index, str.size());
    }
    static const int npos = -1;
};

#endif
