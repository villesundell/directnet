/*
 * Copyright 2004, 2005, 2006, 2007  Gregor Richards
 * Copyright 2006  Bryan Donlan
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

#ifndef DN_DIRECTNET_H
#define DN_DIRECTNET_H

#include <map>
#include <string>

extern "C" {
#include <sys/time.h>
}

#include "directnet/binseq.h"
#include "directnet/globals.h"
#include "directnet/route.h"

// protocol version
#define PROTO_MAJOR 1
#define PROTO_MAJOR_STR "\x00\x01"
#define PROTO_MINOR 65011
#define PROTO_MINOR_STR "\xFD\xF3"

namespace DirectNet {
    extern int serv_port;

    // our name
    extern char dn_name[DN_NAME_LEN+1];

    // connections by encryption key
    extern std::map<BinSeq, void *> dn_conn;

    // encryption keys by hashes
    extern std::map<BinSeq, BinSeq> dn_kbh;

    // names by encryption keys and vice-versa
    extern std::map<BinSeq, BinSeq> dn_names;
    extern std::map<BinSeq, BinSeq> dn_keys;
  
    // route by encryption key
    extern std::map<BinSeq, Route *> dn_routes;

    // has a key been used yet?
    extern std::map<std::string, int> dn_trans_keys;

    // have we seen this user?  When? (by hashed enc key)
    extern std::map<BinSeq, time_t> dn_seen_user;

    // our position in the transkey list
    extern int currentTransKey;

    // is the ui loaded?
    extern char uiLoaded;

    // our IP
    extern char dn_localip[24];

    // stuff for relocatability
    extern char *homedir, *bindir;

    /* validate the chosen dn_name
     * returns: true if it's valid */
    bool validateName();

    /* get the generic form of the name (currently just means lowercase)
     * name: the original name
     * returns the generic name */
    BinSeq genericName(const BinSeq &name);

    /* make a transaction key (unused) */
    void newTransKey(char *into);

    /* see a route of users
     * us: the route */
    void seeUsers(const Route &us);

    /* initialize DN
     * takes same arguments as main() */
    void dn_init(int argc, char **argv);

    /* go online */
    void dn_goOnline();

    /* add a route
     * to: the key of the target
     * rt: the route to it
     * effect: the route is added if it's the shortest available route */
    void dn_addRoute(const BinSeq &to, const Route &rt);
}

//Release notes for this version (if this one is not defined, there is no startup message.
#define DN_RELEASENOTES "<h1>DirectNet</h1>\
<h2>&nbsp;2.0a9</h2>\
<h1>Release Notes</h1>\
This is an alpha version of DirectNet, which means it is <u>only for testing \
purposes</u>, and is not stable enough for daily use. If you find any bugs, \
please contact the developers. You can find contact information at DirectNet's \
homepage, <b>directnet.sf.net</b><br>\
DirectNet is distributed under the GNU General Public License\
"
#define DN_RELEASENOTES_NOHTML "DirectNet 2.0a9\n\
Release Notes\n\
This is an alpha version of DirectNet, which means it is only for testing\n\
purposes, and is not stable enough for daily use. If you find any bugs, please\n\
contact the developers. You can find contact information at DirectNet's\n\
homepage, directnet.sf.net\n\n\
DirectNet is distributed under the GNU General Public License\
"

// First-time question
#define DN_FIRSTTIME "\
This appears to be the first time you've used DirectNet. Would you like to\n\
connect to the primary DirectNet network? (via hub.imdirect.net)\
"
#define DN_FIRSTTIME_HUB "hub.imdirect.net" "<h1>DirectNet</h1>\
<h2>&nbsp;2.0a9</h2>\
<h1>Release Notes</h1>\
This is an alpha version of DirectNet, which means it is <u>only for testing \
purposes</u>, and is not stable enough for daily use. If you find any bugs, \
please contact the developers. You can find contact information at DirectNet's \
homepage, <b>directnet.sf.net</b><br>\
DirectNet is distributed under the GNU General Public License\
"
#define DN_FIRSTTIME_HUBS "hub.imdirect.net\nhub2.imdirect.net\n"

#endif // DN_DIRECTNET_H
