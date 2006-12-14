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

#ifndef DN_BINSEQ_H
#define DN_BINSEQ_H

#include <string>
#include <vector>
using namespace std;

extern "C" {
#include <string.h>
}

class BinSeq : public vector<char> {
    public:
    BinSeq();
    BinSeq(const BinSeq &copy);
    BinSeq(const string &copy);
    BinSeq(const char *copy);
    BinSeq(const char *copy, int len);
    
    // string-like functionality:
    inline const char *c_str() const { return &front(); }
    inline char *c_str() { return &front(); }
    int find(const char* str, int index, int length) const;
    inline int find(char ch, int index) const
    {
        return find(&ch, index, 1);
    }
    inline int find(const char* str, int index) const
    {
        return find(str, index, strlen(str));
    }
    inline int find(const string& str, int index) const
    {
        return find(str.c_str(), index);
    }
    inline int find(BinSeq& str, int index) const
    {
        return find(str.c_str(), index, str.size());
    }
    void push_back(const char *str, int length);
    void push_back(const char *str);
    inline void push_back(const BinSeq &str) { push_back(str.c_str(), str.size()); }
    inline void push_back(const string &str) { push_back(str.c_str()); }
    inline void push_back(const char chr) {
        ((vector<char> *) this)->pop_back();
        ((vector<char> *) this)->push_back(chr);
        ((vector<char> *) this)->push_back(0);
    }
    inline void pop_back() {
        ((vector<char> *) this)->pop_back();
        ((vector<char> *) this)->push_back(0);
    }
    BinSeq substr(int s) const;
    BinSeq substr(int s, int l) const;
    
    static const int npos = -1;
    
    // operators
    inline bool operator==(const BinSeq &comp) const { return (*((vector <char> *) this)) == comp; }
    inline bool operator!=(const BinSeq &comp) const { return (*((vector <char> *) this)) != comp; }
    inline BinSeq &operator+=(const BinSeq &add) { push_back(add.c_str(), add.size()); }
    inline BinSeq &operator+=(const string &add) { push_back(add.c_str()); return *this; }
    inline BinSeq &operator+=(char *add) { push_back(add); return *this; }
    inline BinSeq &operator+=(char add) { push_back(add); return *this; }
    inline char &operator[](int i) const { return (*((vector <char> *) this))[i]; }
    inline operator string() const {
        string a(c_str(), size());
        return a;
    }
    inline BinSeq operator+(const BinSeq &add) const {
        BinSeq out;
        out.push_back(*this);
        out.push_back(add);
        return out;
    }
    
    // accessors
    inline int size() const { return ((vector <char> *) this)->size() - 1; }
};

#endif
