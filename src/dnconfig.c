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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "dnconfig.h"
#include "lock.h"
#include "globals.h"

extern int errno;

char *dn_ac_list_f;
char *dn_ac_list[DN_MAX_CONNS+1];
char *dn_af_list_f;
char *dn_af_list[DN_MAX_ROUTES+1];
DN_LOCK dn_ac_lock;
DN_LOCK dn_af_lock;

/* initConfig
 * Input: none
 * Output: none
 * Effect: configuration files are opened and read
 */
void initConfig()
{
    int i, osl;
    char *cfgdir;
    FILE *acf, *aff;
    
    // find configuration directory
    cfgdir = (char *) malloc(strlen(homedir) + 12);
    if (!cfgdir) { perror("malloc"); exit(1); }
#ifndef __WIN32
    sprintf(cfgdir, "%s/.directnet", homedir);
#else
    sprintf(cfgdir, "%s/DNConfig", homedir);
#endif
    // attempt to create it
    if (mkdir(cfgdir, 0777) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            exit(1);
        }
    }
    
    // find configuration files
    dn_ac_list_f = (char *) malloc(strlen(cfgdir) + 4);
    if (!dn_ac_list_f) { perror("malloc"); exit(1); }
    sprintf(dn_ac_list_f, "%s/ac", cfgdir);
    
    dn_af_list_f = (char *) malloc(strlen(cfgdir) + 4);
    if (!dn_af_list_f) { perror("malloc"); exit(1); }
    sprintf(dn_af_list_f, "%s/af", cfgdir);
    
    // read configuration files
    acf = fopen(dn_ac_list_f, "r");
    if (acf) {
        char line[DN_NAME_LEN+1];
        line[DN_NAME_LEN] = '\0';
        i = 0;
        
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
                dn_ac_list[i] = strdup(line);
                i++;
                if (i >= DN_MAX_CONNS) i--;
            }
        }
        i++;
        dn_ac_list[i] = NULL;
        fclose(acf);
    }
    
    aff = fopen(dn_af_list_f, "r");
    if (aff) {
        char line[DN_HOSTNAME_LEN+1];
        line[DN_HOSTNAME_LEN] = '\0';
        i = 0;
        
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
                dn_af_list[i] = strdup(line);
                i++;
                if (i >= DN_MAX_ROUTES) i--;
            }
        }
        i++;
        dn_af_list[i] = NULL;
        fclose(aff);
    }
    
    // init the locks
    dn_lockInit(&dn_ac_lock);
    dn_lockInit(&dn_af_lock);
    
    free(cfgdir);
}

/* autoConnect
 * Input: none
 * Output: none
 * Effect: connections are attempted to all autoconnect hostnames
 */
void autoConnect()
{
    int i;
    
    dn_lock(&dn_ac_lock);
    
    for (i = 0; i < DN_MAX_CONNS && dn_ac_list[i]; i++) {
        establishClient(dn_ac_list[i]);
    }
    
    dn_unlock(&dn_ac_lock);
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
    int i;
    
    dn_lock(&dn_af_lock);
    
    for (i = 0; i < DN_MAX_ROUTES && dn_af_list[i]; i++) {
        sendFnd(dn_af_list[i]);
    }
    
    dn_unlock(&dn_af_lock);
}

/* addAutoConnect
 * Input: a hostname to autoconnect to in the future
 * Output: none
 * Effect: the hostname is added to the list and the file
 */
void addAutoConnect(const char *hostname)
{
    FILE *outf;
    int i;
    
    dn_lock(&dn_ac_lock);
    
    for (i = 0; i < DN_MAX_CONNS && dn_ac_list[i]; i++);
    if (i == DN_MAX_CONNS) {
        dn_unlock(&dn_ac_lock);
        return;
    }
    
    // now add it to the list
    dn_ac_list[i] = strdup(hostname);
    dn_ac_list[i+1] = NULL;
    
    // and the file
    outf = fopen(dn_ac_list_f, "a");
    if (!outf) {
        dn_unlock(&dn_ac_lock);
        return;
    }
    fprintf(outf, "%s\n", hostname);
    fclose(outf);
    
    dn_unlock(&dn_ac_lock);
}

/* addAutoFind
 * Input: a nick to autofind in the future
 * Output: none
 * Effect: the nick is added to the list and the file
 */
void addAutoFind(const char *nick)
{
    FILE *outf;
    int i;
    
    dn_lock(&dn_af_lock);
    
    for (i = 0; i < DN_MAX_ROUTES && dn_af_list[i]; i++);
    if (i == DN_MAX_ROUTES) {
        dn_unlock(&dn_af_lock);
        return;
    }
    
    // now add it to the list
    dn_af_list[i] = strdup(nick);
    dn_af_list[i+1] = NULL;
    
    // and the file
    outf = fopen(dn_af_list_f, "a");
    if (!outf) {
        dn_unlock(&dn_af_lock);
        return;
    }
    fprintf(outf, "%s\n", nick);
    fclose(outf);
    
    dn_unlock(&dn_af_lock);
}

/* remAutoConnect
 * Input: a hostname to remove from the autoconnect list
 * Output: none
 * Effect: it is removed from the list and file
 */
void remAutoConnect(const char *hostname)
{
    FILE *outf;
    int i;
    
    dn_lock(&dn_ac_lock);
    
    for (i = 0;
         i < DN_MAX_CONNS &&
         dn_ac_list[i] &&
         strcmp(dn_ac_list[i], hostname);
         i++);
    if (i == DN_MAX_CONNS) {
        dn_unlock(&dn_ac_lock);
        return;
    }
    
    free(dn_ac_list[i]);
    
    // in the list, move them down
    for (; i < DN_MAX_CONNS && dn_ac_list[i]; i++) {
        dn_ac_list[i] = dn_ac_list[i+1];
    }
    
    // then write out the file
    outf = fopen(dn_ac_list_f, "w");
    if (!outf) {
        dn_unlock(&dn_ac_lock);
        return;
    }
    
    for (i = 0; i < DN_MAX_CONNS && dn_ac_list[i]; i++) {
        fprintf(outf, "%s\n", dn_ac_list[i]);
    }
    
    fclose(outf);
    
    dn_unlock(&dn_ac_lock);
}

/* remAutoFind
 * Input: a nick to remove from the autofind list
 * Output: none
 * Effect: it is removed from the list and file
 */
void remAutoFind(const char *nick)
{
    FILE *outf;
    int i;
    
    dn_lock(&dn_af_lock);
    
    for (i = 0;
         i < DN_MAX_ROUTES &&
         dn_af_list[i] &&
         strcmp(dn_af_list[i], nick);
         i++);
    if (i == DN_MAX_ROUTES) {
        dn_unlock(&dn_af_lock);
        return;
    }
    
    free(dn_af_list[i]);
    
    // in the list, move them down
    for (; i < DN_MAX_ROUTES && dn_af_list[i]; i++) {
        dn_af_list[i] = dn_af_list[i+1];
    }
    
    // then write out the file
    outf = fopen(dn_af_list_f, "w");
    if (!outf) {
        dn_unlock(&dn_af_lock);
        return;
    }
    
    for (i = 0; i < DN_MAX_ROUTES && dn_af_list[i]; i++) {
        fprintf(outf, "%s\n", dn_af_list[i]);
    }
    
    fclose(outf);
    
    dn_unlock(&dn_af_lock);
}

/* checkAutoConnect
 * Input: a hostname
 * Output: 1 if it is in the autoconnect list
 *         0 otherwise
 * Effect: none
 */
char checkAutoConnect(const char *hostname)
{
    int i;
    
    dn_lock(&dn_ac_lock);
    
    for (i = 0;
         i < DN_MAX_CONNS &&
         dn_ac_list[i] &&
         strcmp(dn_ac_list[i], hostname);
         i++);
    
    dn_unlock(&dn_ac_lock);
    
    if (i == DN_MAX_CONNS || !dn_ac_list[i]) return 0;
    return 1;    
}

/* checkAutoFind
 * Input: a nick
 * Output: 1 if it is in the autofind list
 *         0 otherwise
 * Effect: none
 */
char checkAutoFind(const char *nick)
{
    int i;
    
    dn_lock(&dn_af_lock);
    
    for (i = 0;
         i < DN_MAX_ROUTES &&
         dn_af_list[i] &&
         strcmp(dn_af_list[i], nick);
         i++);
    
    dn_unlock(&dn_af_lock);
    
    if (i == DN_MAX_ROUTES || !dn_af_list[i]) return 0;
    return 1;    
}
