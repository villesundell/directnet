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

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "route.h"

Route::Route() : vector<BinSeq>() {}

Route::Route(const Route &copy) : vector<BinSeq>(copy) {}

Route::Route(const BinSeq &textform) : vector<BinSeq>()
{
    vector<int> elemlens;
    int i, cur;
    
    for (i = 0; i < textform.size(); i++) {
        cur = (unsigned char) textform[i];
        if (cur == 255) break;
        elemlens.push_back(cur);
    }
    i++;
    
    for (int j = 0; j < elemlens.size() && i < textform.size(); j++) {
        if (textform.size() - i >= elemlens[j]) {
            push_back(textform.substr(i, elemlens[j]));
            i += elemlens[j];
        }
    }
}
    
BinSeq Route::toBinSeq()
{
    BinSeq toret;
    int s = size(), i;
    
    for (i = 0; i < s; i++) {
        toret.push_back((unsigned char) at(i).size());
    }
    toret.push_back(255);
    
    for (i = 0; i < s; i++) {
        toret.push_back(at(i));
    }
    
    return toret;
}

void Route::reverse()
{
    int i, j, s = size();
    BinSeq temp;
    
    j = s - 1;
    for (i = 0; i < j; j = s - ++i - 1) {
        temp = at(j);
        at(j) = at(i);
        at(i) = temp;
    }
}

void Route::push_front(const BinSeq &a)
{
    int i, s;
    
    push_back("");
    
    s = size();
    
    for (i = s - 1; i > 0; i--) {
        at(i) = at(i - 1);
    }
    
    at(0) = a;
}

void Route::pop_front()
{
    int i, s;
    
    s = size();
    
    for (i = 0; i < s - 1; i++) {
        at(i) = at(i + 1);
    }
    
    pop_back();
}

void Route::append(const Route &a)
{
    for (int i = 0; i < a.size(); i++) {
        push_back(a[i]);
    }
}

bool Route::find(const BinSeq &a) const
{
    for (int i = 0; i < size(); i++) {
        if (at(i) == a) return true;
    }
    return false;
}

void Route::debug()
{
    cout << "ROUTE:" << endl;
    for (int i = 0; i < size(); i++) {
        cout << " " << i << ": ";
        for (int j = 0; j < at(i).size(); j++) {
            printf("%.2X", (unsigned char) at(i)[j]);
        }
        cout << endl;
    }
}
