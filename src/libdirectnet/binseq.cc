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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#include "directnet/binseq.h"
using namespace std;

namespace DirectNet {
    BinSeq::BinSeq()
    {
        ((vector<char> *) this)->push_back(0);
    }

    BinSeq::BinSeq(const BinSeq &copy) : vector<char>(copy) {}

    BinSeq::BinSeq(const string &copy)
    {
        ((vector<char> *) this)->push_back(0);
        for (int i = 0; i < copy.length(); i++) {
            push_back(copy[i]);
        }
    }

    BinSeq::BinSeq(const char *copy)
    {
        ((vector<char> *) this)->push_back(0);
        push_back(copy);
    }

    BinSeq::BinSeq(const char *copy, int len)
    {
        ((vector<char> *) this)->push_back(0);
        push_back(copy, len);
    }

    BinSeq BinSeq::substr(int s) const
    {
        BinSeq tr;
    
        for (int i = s; i < size(); i++) {
            tr.push_back((*this)[i]);
        }
    
        return tr;
    }

    BinSeq BinSeq::substr(int s, int l) const
    {
        BinSeq tr;
    
        for (int i = s; i < s + l && i < size(); i++) {
            tr.push_back((*this)[i]);
        }
    
        return tr;
    }

    int BinSeq::find(const char* str, int index, int length) const
    {
        const char *buf = c_str();
    
        for (int i = 0; i < size() - length; i++) {
            if (!strncmp((char *) buf + i, (char *) str, length)) return i;
        }
    
        return npos;
    }

    void BinSeq::push_back(const char *str, int length)
    {
        for (int i = 0; i < length; i++) {
            push_back(str[i]);
        }
    }

    void BinSeq::push_back(const char *str)
    {
        for (int i = 0; str[i]; i++) {
            push_back(str[i]);
        }
    }
}
