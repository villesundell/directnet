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

#include <string>
#include <vector>
using namespace std;

#include "route.h"

Route::Route() : vector<string>() {}

Route::Route(const Route &copy) : vector<string>(copy) {}

Route::Route(const string &textform) : vector<string>()
{
    int x = 0, y;
    while (x < textform.length() &&
           (y = textform.find_first_of('\n', x)) != string::npos) {
        if (y - x > 0)
            push_back(textform.substr(x, y - x));
        x = y + 1;
    }
}
    
string Route::toString()
{
    string toret;
    int s = size();
    
    for (int i = 0; i < s; i++) {
        toret += at(i) + "\n";
    }
    
    return toret;
}

void Route::reverse()
{
    int i, j, s = size();
    string temp;
    
    j = s - 1;
    for (i = 0; i < j; j = s - ++i - 1) {
        temp = at(j);
        at(j) = at(i);
        at(i) = temp;
    }
}

void Route::push_front(string &a)
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
