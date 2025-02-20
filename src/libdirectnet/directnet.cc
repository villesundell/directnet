/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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

#include <map>
#include <string>
using namespace std;

extern "C" {
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <unistd.h>
}

#include "config.h"

#include "directnet/chat.h"
#include "directnet/compat.h"
#include "directnet/connection.h"
#include "directnet/directnet.h"
#include "directnet/dnconfig.h"
#include "directnet/enc.h"
#include "directnet/globals.h"
#include "directnet/message.h"
#include "directnet/route.h"
#include "directnet/server.h"
#include "directnet/ui.h"
#include "directnet/whereami.h"

extern char **environ; // XXX: should use getenv

namespace DirectNet {
    int serv_port = 3447;

    char dn_name[DN_NAME_LEN+1];

    map<BinSeq, void *> dn_conn;
    map<BinSeq, BinSeq> dn_kbh;
    map<BinSeq, BinSeq> dn_names;
    map<BinSeq, BinSeq> dn_keys;

    map<BinSeq, Route *> dn_routes;

    map<string, int> dn_trans_keys;
    int currentTransKey;

    map<BinSeq, time_t> dn_seen_user;

    char uiLoaded;

    char dn_localip[24];

    char *homedir, *bindir;

    char *findHome(char **envp);

    /* validate the chosen dn_name
     * returns: true if it's valid */
    bool validateName()
    {
        // valid names: [a-zA-Z0-9_\-\[\]]
        int i;
        for (i = 0; dn_name[i]; i++) {
            char c = dn_name[i];
            if ((c < 'a' || c > 'z') &&
                (c < 'A' || c > 'Z') &&
                (c < '0' || c > '9') &&
                (c != '_') && (c != '-') &&
                (c != '[') && (c != ']'))
                return false;
        }
        if (i == 0) return false;
        return true;
    }

    /* get the generic form of the name (currently just means lowercase)
     * name: the original name
     * returns the generic name */
    BinSeq genericName(const BinSeq &name)
    {
        BinSeq ret;
        for (int i = 0; i < name.size(); i++) {
            char c = name[i];
            if (c >= 'A' && c <= 'Z') {
                c += ('a' - 'A');
            }
            ret.push_back(c);
        }
        return ret;
    }

    /* initialize DN
     * takes same arguments as main() */
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
                } else if (!strncmp(argv[i], "-m", 4)) {
                    /* this is just here because OSX is weird */
                } else if (!strncmp(argv[i], "-n", 2)) {
                    /* this is just here so the frontend can use it */
                    i++;
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
                    fprintf(stderr, "  -p: Set the port to listen for connections on (default 3447).\n\n");
                    exit(1);
                }
            }
        
        }
    
        // Start with transkey 0
        currentTransKey = 0;
    
        // We don't yet know our local IP
        dn_localip[0] = '\0';
    
        // Set home directory
        homedir = strdup(findHome(environ));
    
        // initialize configuration
        initConfig();
    }

    /* go online */
    void dn_goOnline() {
        // generate the key
        encCreateKey();
    
        // no server for NESTEDVM
#ifndef NESTEDVM 
        establishServer();
#endif
    
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
   
#if defined(WIN32) || defined(NESTEDVM)
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
        dn_trans_keys[string(into)] = 1;
        currentTransKey++;
    }

    /* see a route of users
     * us: the route */
    void seeUsers(const Route &us)
    {
        // get the current time
        struct timeval tv;
        gettimeofday(&tv, NULL);
    
        int day = tv.tv_sec - (60 * 60 * 24);
    
        // see the users
        for (int i = 0; i < us.size(); i++) {
            dn_seen_user[us[i]] = tv.tv_sec;
        }
    
        // and flush out any users we haven't seen in 24 hours
        map<BinSeq, time_t>::iterator sui;
        flushusers:
        for (sui = dn_seen_user.begin(); sui != dn_seen_user.end(); sui++) {
            if (sui->second < day) {
                dn_seen_user.erase(sui);
                goto flushusers;
            }
        }
    }

    /* add a route
     * to: the key of the target
     * rt: the route to it
     * effect: the route is added if it's the shortest available route */
    void dn_addRoute(const BinSeq &to, const Route &rt)
    {
        // add a route intelligently, "short-circuit" to the shortest known-possible route
        Route *nroute = new Route(rt);
        for (int i = nroute->size() - 1; i > 0; i--) {
            if (dn_kbh.find((*nroute)[i]) != dn_kbh.end()) {
                BinSeq &key = dn_kbh[(*nroute)[i]];
            
                // we have their key, do we have a connection?
                if (dn_conn.find(key) != dn_conn.end()) {
                    // yes!  Cut down the route
                    for (; i > 0; i--) nroute->pop_front();
                }
            }
        }
    
        // now that we have an efficient route, is it more efficient than the alternative?
        if (dn_routes.find(to) != dn_routes.end()) {
            Route *oroute = dn_routes[to];
            if (oroute->size() > nroute->size()) {
                // old route is longer, drop it
                delete oroute;
            } else {
                // just keep the old one
                delete nroute;
                return;
            }
        }
    
        dn_routes[to] = nroute;
    
        // send a pir
        Message msg(1, "pir", 1, 1);
        msg.params.push_back(nroute->toBinSeq());
    
        Route rroute = *nroute;
        if (rroute.size()) rroute.pop_back();
        rroute.reverse();
        rroute.push_back(pukeyhash);
        msg.params.push_back(rroute.toBinSeq());
    
        handleRoutedMsg(msg);
    }
}
