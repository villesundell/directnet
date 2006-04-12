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

#include "binseq.h"

BinSeq::BinSeq() : vector<unsigned char>() {}

BinSeq::BinSeq(const BinSeq &copy) : vector<unsigned char>(copy) {}

BinSeq::BinSeq(const string &copy) : vector<unsigned char>()
{
    for (int i = 0; i < copy.length(); i++) {
        push_back(copy[i]);
    }
}

BinSeq::BinSeq(const unsigned char *copy, int len) : vector<unsigned char>()
{
    for (int i = 0; i < len; i++) {
        push_back(copy[i]);
    }
}

int BinSeq::find(const unsigned char* str, int index, int length)
{
    unsigned char *buf = c_str();
    
    for (int i = 0; i < size() - length; i++) {
        if (!strcmp(buf + i, str, length)) return i;
    }
    
    return npos;
}
