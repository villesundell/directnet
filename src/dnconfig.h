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

#ifndef DN_DNCONFIG_H
#define DN_DNCONFIG_H

#include <set>
#include <string>
using namespace std;

// the directory with configuration info
extern string cfgdir;

// configuration functions

/* initConfig
 * Input: none
 * Output: none
 * Effect: configuration files are opened and read
 */
void initConfig();

/* autoConnect
 * Input: none
 * Output: none
 * Effect: connections are attempted to all autoconnect hostnames
 */
void autoConnect();

/* autoConnectThread
 * Input: none
 * Output: none
 * Effect: autoconnect in a thread
 */
void *autoConnectThread(void *ignore);

/* autoFind
 * Input: none
 * Output: none
 * Effect: finds are sent to all autofind nicks
 */
void autoFind();

/* addAutoConnect
 * Input: a hostname to autoconnect to in the future
 * Output: none
 * Effect: the hostname is added to the list and the file
 */
void addAutoConnect(const string &hostname);

/* addAutoFind
 * Input: a nick to autofind in the future
 * Output: none
 * Effect: the nick is added to the list and the file
 */
void addAutoFind(const string &nick);

/* remAutoConnect
 * Input: a hostname to remove from the autoconnect list
 * Output: none
 * Effect: it is removed from the list and file
 */
void remAutoConnect(const string &hostname);

/* remAutoFind
 * Input: a nick to remove from the autofind list
 * Output: none
 * Effect: it is removed from the list and file
 */
void remAutoFind(const string &nick);

/* checkAutoConnect
 * Input: a hostname
 * Output: 1 if it is in the autoconnect list
 *         0 otherwise
 * Effect: none
 */
bool checkAutoConnect(const string &hostname);

/* checkAutoFind
 * Input: a nick
 * Output: 1 if it is in the autofind list
 *         0 otherwise
 * Effect: none
 */
bool checkAutoFind(const string &nick);

// our configuration files
extern string dn_ac_list_f;
extern set<string> *dn_ac_list;
extern string dn_af_list_f;
extern set<string> *dn_af_list;

#endif
