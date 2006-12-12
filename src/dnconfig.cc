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
#include "directnet.h"
#include "dnconfig.h"
#include "globals.h"

extern int errno;

string dn_ac_list_f;
set<string> *dn_ac_list;
string dn_af_list_f;
set<string> *dn_af_list;
string cfgdir;

/* initConfig
 * Input: none
 * Output: none
 * Effect: configuration files are opened and read
 */
void initConfig()
{
    int i, osl;
    FILE *acf, *aff;
    
    // find configuration directory
#ifndef __WIN32
    cfgdir = homedir + string("/.directnet2");
#else
    cfgdir = homedir + string("/DN2Config");
#endif
    
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
    dn_af_list = new set<string>;
    
    // find configuration files
    dn_ac_list_f = cfgdir + "/ac";
    dn_af_list_f = cfgdir + "/af";
    
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
}

/* autoConnect
 * Input: none
 * Output: none
 * Effect: connections are attempted to all autoconnect hostnames
 */
void autoConnect()
{
    set<string>::iterator aci;
    
    for (aci = dn_ac_list->begin(); aci != dn_ac_list->end(); aci++) {
        async_establishClient(*aci);
    }
    
}

/* autoConnectThread
 * Input: none
 * Output: none
 * Effect: autoconnect in a thread
 */
void *autoConnectThread(void *ignore)
{
    autoConnect();
    return NULL;
}

/* autoFind
 * Input: none
 * Output: none
 * Effect: finds are sent to all autofind nicks
 */
void autoFind()
{
    set<string>::iterator afi;
    
    for (afi = dn_af_list->begin(); afi != dn_af_list->end(); afi++) {
        sendFnd(*afi);
    }
}

/* addAutoConnect
 * Input: a hostname to autoconnect to in the future
 * Output: none
 * Effect: the hostname is added to the list and the file
 */
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

/* addAutoFind
 * Input: a nick to autofind in the future
 * Output: none
 * Effect: the nick is added to the list and the file
 */
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
 * Input: a hostname to remove from the autoconnect list
 * Output: none
 * Effect: it is removed from the list and file
 */
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
 * Input: a nick to remove from the autofind list
 * Output: none
 * Effect: it is removed from the list and file
 */
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
 * Input: a hostname
 * Output: 1 if it is in the autoconnect list
 *         0 otherwise
 * Effect: none
 */
bool checkAutoConnect(const string &hostname)
{
    if (dn_ac_list->find(hostname) != dn_ac_list->end()) return true;
    return false;
}

/* checkAutoFind
 * Input: a nick
 * Output: 1 if it is in the autofind list
 *         0 otherwise
 * Effect: none
 */
bool checkAutoFind(const string &nick)
{
    if (dn_af_list->find(nick) != dn_af_list->end()) return true;
    return false;
}
