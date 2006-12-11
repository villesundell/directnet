/*
 * Copyright 2004, 2005, 2006  Gregor Richards
 * Copyright 2006 Bryan Donlan
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

#ifndef DN_DIRECTNET_H
#define DN_DIRECTNET_H

#include <map>
#include <string>
using namespace std;

#include <sys/time.h>

#include "binseq.h"
#include "globals.h"
#include "route.h"

extern int serv_port;

// our name
extern char dn_name[DN_NAME_LEN+1];

// connections by encryption key
extern map<BinSeq, void *> *dn_conn;

// encryption keys by hashes
extern map<BinSeq, BinSeq> *dn_kbh;

// names by encryption keys and vice-versa
extern map<BinSeq, BinSeq> *dn_names;
extern map<BinSeq, BinSeq> *dn_keys;
  
// route by encryption key
extern map<BinSeq, Route *> *dn_routes;

// has a key been used yet?
extern map<string, int> *dn_trans_keys;

// have we seen this user?  When? (by hashed enc key)
extern map<BinSeq, time_t> *dn_seen_user;

// our position in the transkey list
extern int currentTransKey;

// is the ui loaded?
extern char uiLoaded;

// our IP
extern char dn_localip[24];

// stuff for relocatability
extern char *homedir, *bindir;

void newTransKey(char *into);

// see a route of users
void seeUsers(const Route &us);

void dn_init(int argc, char **argv);
void dn_goOnline();

// add a route
void dn_addRoute(const BinSeq &to, const Route &rt);

//Release notes for this version (if this one is not defined, there is no startup message.
#define DN_RELEASENOTES "<h1>DirectNet</h1>\
<h2>&nbsp;2.0a1</h2>\
<h1>Release Notes</h1>\
This is a alpha version, which means this version is <u>only for testing \
purposes</u>, and is not stable enough for daily use. If you find any bugs, \
please contact the developers. You can find contact information at DirectNet's \
homepage, <b>directnet.sf.net</b><br>\
DirectNet is distributed under the GNU General Public License\
"

#endif // DN_DIRECTNET_H
