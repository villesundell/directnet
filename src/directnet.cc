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

#include <map>
#include <string>
using namespace std;

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <unistd.h>

#include "chat.h"
#include "config.h"
#include "directnet.h"
#include "dnconfig.h"
#include "enc.h"
#include "globals.h"
#include "route.h"
#include "server.h"
#include "ui.h"
#include "whereami.h"

extern char **environ; // XXX: should use getenv

int serv_port = 3336;

char dn_name[DN_NAME_LEN+1];

map<string, void *> *dn_conn;

map<string, Route *> *dn_routes;
map<string, Route *> *dn_iRoutes;

map<string, int> *dn_trans_keys;
int currentTransKey;

char uiLoaded;

char dn_localip[24];

char *homedir, *bindir;

char *findHome(char **envp);

void dn_init(int argc, char **argv) {
    int i;
    char *binloc, *binnm;
    
    // check our installed path
    binloc = whereAmI(argc ? argv[0] : "directnet", &bindir, &binnm);
    if (binloc) {
        // we only need bindir
        free(binloc);
        free(binnm);
    } else {
        // make it up :(
        bindir = strdup("/usr/bin");
    }
    
    if (argc >= 2) {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[1], "-v", 2)) {
                printf("DirectNet version %s\n", VERSION);
                exit(0);
            } else if (!strncmp(argv[i], "-psn", 4)) {
                /* this is just here because OSX is weird */
            } else if (!strncmp(argv[i], "-p", 2)) {
                i++;
                if (!argv[i]) {
                    fprintf(stderr, "-p must have an argument\n");
                    exit(1);
                }
                serv_port = atoi(argv[i]);
            } else {
                fprintf(stderr, "Use:\n%s [-v] [-p port]\n", argv[0]);
                fprintf(stderr, "  -v: Display the version number and quit.\n");
                fprintf(stderr, "  -p: Set the port to listen for connections on (default 3336).\n\n");
                exit(1);
            }
        }
        
    }
    
    // This stores connections by name
    dn_conn = new map<string, void *>;
      
    // This stores routes by name
    dn_routes = new map<string, Route *>;
    // This stores intermediate routes, for response on broken routes
    dn_iRoutes = new map<string, Route *>;
    
    // This hash stores the state of all nonrepeating unrouted messages
    dn_trans_keys = new map<string, int>;
    currentTransKey = 0;
    
    // This hash stores whether we're in certain chats
    dn_chats = new map<string, vector<string> *>;
    
    // We don't yet know our local IP
    dn_localip[0] = '\0';
    
    // Set home directory
    homedir = strdup(findHome(environ));
    
    // initialize configuration
    initConfig();
}
    
void dn_goOnline() {
    establishServer();
    
    autoConnect();
    
    return;
}

char *findHome(char **envp)
{
    int i;
    
    for (i = 0; envp[i] != NULL; i++) {
        if (!strncmp(envp[i], "HOME=", 5)) {
            return envp[i]+5;
        }
    }
   
#ifdef WIN32
    // On Windoze we'll accept curdir
    {
        char *cdir = (char *) malloc(1024);
        if (!cdir) { perror("malloc"); exit(1); }
        if (getcwd(cdir, 1024))
            return cdir;
        // if no curdir, just use bindir
        return bindir;
    }
#endif
    
    fprintf(stderr, "Couldn't find your HOME directory.\n");
    exit(-1);
}

void newTransKey(char *into)
{
    sprintf(into, "%s%d", dn_name, currentTransKey);
    (*dn_trans_keys)[string(into)] = 1;
    currentTransKey++;
}
