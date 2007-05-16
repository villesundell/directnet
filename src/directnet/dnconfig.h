/*
 * Copyright 2006, 2007  Gregor Richards
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

#ifndef DN_DNCONFIG_H
#define DN_DNCONFIG_H

#include <list>
#include <set>
#include <string>

namespace DirectNet {
    // the directory with configuration info
    extern string cfgdir;

    // configuration functions

    /* initConfig
     * effect: configuration files are opened and read */
    void initConfig();

    /* autoConnect
     * effect: connections are attempted to all autoconnect and autoreconnect
     *         hostnames */
    void autoConnect();

    /* autoFind
     * effect: finds are sent to all autofind nicks */
    void autoFind();

    /* addAutoConnect
     * hostname: a hostname to autoconnect to in the future
     * effect: the hostname is added to the list and the file */
    void addAutoConnect(const string &hostname);

    /* addAutoReconnect
     * hostname: a hostname to reconnect to in the future
     * effect: the hostname is added to the list and the file */
    void addAutoReconnect(const string &hostname);

    /* addAutoFind
     * nick: a nick to autofind in the future
     * effect: the nick is added to the list and the file */
    void addAutoFind(const string &nick);

    /* remAutoConnect
     * hostname: a hostname to remove from the autoconnect list
     * effect: it is removed from the list and file */
    void remAutoConnect(const string &hostname);

    /* remAutoFind
     * nick: a nick to remove from the autofind list
     * effect: it is removed from the list and file */
    void remAutoFind(const string &nick);

    /* checkAutoConnect
     * hostname: a hostname
     * returns 1 if it is in the autoconnect list, 0 otherwise */
    bool checkAutoConnect(const string &hostname);

    /* checkAutoFind
     * nick: a nick
     * returns 1 if it is in the autofind list, 0 otherwise */
    bool checkAutoFind(const string &nick);

    /* saveNick
     * effect: the current nick is cached */
    void saveNick();

    // our configuration files
    extern string dn_ac_list_f;
    extern set<string> *dn_ac_list;
    extern string dn_ar_list_f;
    extern list<string> *dn_ar_list;
    extern string dn_af_list_f;
    extern set<string> *dn_af_list;
}

#endif
