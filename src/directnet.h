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

#include "globals.h"
#include "hash.h"

#ifdef GAIM_PLUGIN
int pluginMain(int argc, char **argv, char **envp);
#endif

extern int serv_port;

extern struct hashS *weakRoutes; // List of weak routes

// our name
extern char dn_name[DN_NAME_LEN+1];

// connections by name
extern struct hashV *dn_conn;
  
// route by name
extern struct hashS *dn_routes;

// intermediate routes by name
extern struct hashS *dn_iRoutes;

// has a key been used yet?
extern struct hashI *dn_trans_keys;

// our position in the transkey list
extern int currentTransKey;

// is the ui loaded?
extern char uiLoaded;

// our IP
extern char dn_localip[24];

// stuff for relocatability
extern char *homedir, *bindir;

void newTransKey(char *into);

void dn_init(int argc, char **argv);
void dn_goOnline();

#endif // DN_DIRECTNET_H
