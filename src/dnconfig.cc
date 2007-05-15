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

#include <set>
#include <string>
using namespace std;

extern "C" {
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __WIN32
#include <io.h>
#endif
}

#include "client.h"
#include "connection.h"
#include "dht.h"
#include "directnet.h"
#include "dn_event.h"
#include "dnconfig.h"
#include "globals.h"
#include "ui.h"

extern int errno;

string dn_ac_list_f;
set<string> *dn_ac_list;
string dn_ar_list_f;
list<string> *dn_ar_list;
string dn_af_list_f;
set<string> *dn_af_list;
string dn_nick_f;
string cfgdir;

/* initConfig
 * effect: configuration files are opened and read */
void initConfig()
{
    int i, osl;
    FILE *acf, *arf, *aff, *nickf;
    
    // find configuration directory
    if (cfgdir == "") { // allow the UI to override
#ifndef __WIN32
        cfgdir = homedir + string("/.directnet2");
#else
        cfgdir = homedir + string("/DN2Config");
#endif
    } else if (
#ifndef __WIN32
        cfgdir[0] != '/' &&
        (cfgdir.length() < 3 || cfgdir[1] != ':' || (cfgdir[2] != '\\' && cfgdir[2] != '/'))
#else
        cfgdir[0] != '/'
#endif
        ) {
        // if it's relative, assume it's under homedir
        cfgdir = homedir + ("/" + cfgdir);
    }
    
    // check if it exists
    bool firsttime = false;
    if (access(cfgdir.c_str(), F_OK) != 0) {
        // first time user!
        firsttime = true;
    }
    
    // attempt to create it
#ifndef __WIN32
    int mdr = mkdir(cfgdir.c_str(), 0777);
#else
    int mdr = mkdir(cfgdir.c_str());
#endif
    if (mdr == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            exit(1);
        }
    }
    
    // start our sets
    dn_ac_list = new set<string>;
    dn_ar_list = new list<string>;
    dn_af_list = new set<string>;
    
    // find configuration files
    dn_ac_list_f = cfgdir + "/ac";
    dn_ar_list_f = cfgdir + "/ar";
    dn_af_list_f = cfgdir + "/af";
    dn_nick_f = cfgdir + "/nick";
    
    // perhaps do first-time actions
    if (firsttime && uiFirstTime()) {
        // generate an auto-connect list
        acf = fopen(dn_ac_list_f.c_str(), "w");
        if (acf) {
            fprintf(acf, "%s", DN_FIRSTTIME_HUBS);
            fclose(acf);
        }
    }
    
    // read configuration files
    acf = fopen(dn_ac_list_f.c_str(), "r");
    if (acf) {
        char line[DN_NAME_LEN+1];
        line[DN_NAME_LEN] = '\0';
        
        while (!feof(acf) && !ferror(acf)) {
            if (fgets(line, DN_NAME_LEN, acf)) {
                // cut off the end
                osl = strlen(line) - 1;
                while (line[osl] == '\n' ||
                       line[osl] == '\r') {
                    line[osl] = '\0';
                    osl--;
                }
                
                // make sure it's longer than 0
                if (!line[0]) continue;
                
                // add to our list
                dn_ac_list->insert(line);
            }
        }
        fclose(acf);
    }
    
    arf = fopen(dn_ar_list_f.c_str(), "r");
    if (arf) {
        char line[DN_NAME_LEN+1];
        line[DN_NAME_LEN] = '\0';
        
        while (!feof(arf) && !ferror(arf)) {
            if (fgets(line, DN_NAME_LEN, arf)) {
                // cut off the end
                osl = strlen(line) - 1;
                while (line[osl] == '\n' ||
                       line[osl] == '\r') {
                    line[osl] = '\0';
                    osl--;
                }
                
                // make sure it's longer than 0
                if (!line[0]) continue;
                
                // add to our list
                dn_ar_list->push_back(line);
            }
        }
        fclose(arf);
        
        // make sure the list is sane
        while (dn_ar_list->size() > 5) {
            dn_ar_list->pop_front();
        }
    }
    
    aff = fopen(dn_af_list_f.c_str(), "r");
    if (aff) {
        char line[DN_HOSTNAME_LEN+1];
        line[DN_HOSTNAME_LEN] = '\0';
        
        while (!feof(aff) && !ferror(aff)) {
            if (fgets(line, DN_HOSTNAME_LEN, aff)) {
                // cut off the end
                osl = strlen(line) - 1;
                while (line[osl] == '\n' ||
                       line[osl] == '\r') {
                    line[osl] = '\0';
                    osl--;
                }
                
                // make sure it's longer than 0
                if (!line[0]) continue;
                
                // add to our list
                dn_af_list->insert(line);
            }
        }
        fclose(aff);
    }
    
    nickf = fopen(dn_nick_f.c_str(), "r");
    if (nickf) {
        fgets(dn_name, DN_NAME_LEN, nickf);
    } else {
        dn_name[0] = '\0';
    }
}

/* autoConnecthost
 * effect: A single connection is attempted */
void autoConnectHost(dn_event_timer *dte)
{
    string *hostname = (string *) dte->payload;
    delete dte;
    async_establishClient(*hostname);
    delete hostname;
}

/* autoConnect
 * effect: connections are attempted to all autoconnect hostnames */
void autoConnect()
{
    set<string>::iterator aci;
    list<string>::iterator ari;
    
    int timeout = 0;
    dn_event_timer *dte;
    
    // first autoconnects
    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
        dte = new dn_event_timer(
            timeout, 0,
            autoConnectHost, new string(*aci)
            );
        dte->activate();
        timeout += 5;
    }
    
    // then autoreconnects
    for (ari = dn_ar_list->begin(); ari != dn_ar_list->end(); ari++) {
        dte = new dn_event_timer(
            timeout, 0,
            autoConnectHost, new string(*ari)
            );
        dte->activate();
        timeout += 5;
    }
}

/* autoFind
 * effect: finds are sent to all autofind nicks */
void autoFind()
{
    set<string>::iterator afi;
    
    for (afi = dn_af_list->begin(); afi != dn_af_list->end(); afi++) {
        sendFnd(*afi);
    }
}

/* addAutoConnect
 * hostname: a hostname to autoconnect to in the future
 * effect: the hostname is added to the list and the file */
void addAutoConnect(const string &hostname)
{
    FILE *outf;
    
    // add it to the list
    dn_ac_list->insert(hostname);
    
    // and the file
    outf = fopen(dn_ac_list_f.c_str(), "a");
    if (!outf) {
        return;
    }
    fprintf(outf, "%s\n", hostname.c_str());
    fclose(outf);
}

/* addAutoReconnect
 * hostname: a hostname to reconnect to in the future
 * effect: the hostname is added to the list and the file */
void addAutoReconnect(const string &hostname)
{
    FILE *outf;
    list<string>::iterator ari;
    
    // add it to the list,
    dn_ar_list->push_back(hostname);
    
    // make sure the list isn't too long,
    while (dn_ar_list->size() > 5) {
        dn_ar_list->pop_front();
    }
    
    // and update the file
    outf = fopen(dn_ar_list_f.c_str(), "w");
    if (!outf) {
        return;
    }
    for (ari = dn_ar_list->begin(); ari != dn_ar_list->end(); ari++) {
        fprintf(outf, "%s\n", ari->c_str());
    }
    fclose(outf);
}

/* addAutoFind
 * nick: a nick to autofind in the future
 * effect: the nick is added to the list and the file */
void addAutoFind(const string &nick)
{
    FILE *outf;
    
    // add it to the list
    dn_af_list->insert(nick);
    
    // and the file
    outf = fopen(dn_af_list_f.c_str(), "a");
    if (!outf) {
        return;
    }
    fprintf(outf, "%s\n", nick.c_str());
    fclose(outf);
}

/* remAutoConnect
 * hostname: a hostname to remove from the autoconnect list
 * effect: it is removed from the list and file */
void remAutoConnect(const string &hostname)
{
    FILE *outf;
    set<string>::iterator aci;
    
    dn_ac_list->erase(hostname);
    
    // then write out the file
    outf = fopen(dn_ac_list_f.c_str(), "w");
    if (!outf) {
        return;
    }
    
    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
        fprintf(outf, "%s\n", aci->c_str());
    }
    
    fclose(outf);
}

/* remAutoFind
 * nick: a nick to remove from the autofind list
 * effect: it is removed from the list and file */
void remAutoFind(const string &nick)
{
    FILE *outf;
    set<string>::iterator afi;
    
    dn_af_list->erase(nick);
    
    // then write out the file
    outf = fopen(dn_af_list_f.c_str(), "w");
    if (!outf) {
        return;
    }
    
    for (afi = dn_af_list->begin(); afi != dn_af_list->end(); afi++) {
        fprintf(outf, "%s\n", afi->c_str());
    }
    
    fclose(outf);
}

/* checkAutoConnect
 * hostname: a hostname
 * returns 1 if it is in the autoconnect list, 0 otherwise */
bool checkAutoConnect(const string &hostname)
{
    if (dn_ac_list->find(hostname) != dn_ac_list->end()) return true;
    return false;
}

/* checkAutoFind
 * nick: a nick
 * returns 1 if it is in the autofind list, 0 otherwise */
bool checkAutoFind(const string &nick)
{
    if (dn_af_list->find(nick) != dn_af_list->end()) return true;
    return false;
}

/* saveNick
 * effect: the current nick is cached */
void saveNick()
{
    FILE *outf;
    outf = fopen(dn_nick_f.c_str(), "w");
    if (!outf) {
        return;
    }
    
    fprintf(outf, "%s", dn_name);
    fclose(outf);
}
