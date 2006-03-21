/*
 * Copyright 2004, 2005  Gregor Richards
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "hash.h"

struct hashS *dn_chats;

char chatOnChannel(const char *channel)
{
    
    if (hashSGet(dn_chats, channel) != NULL) {
        return 1;
    } else {
        return 0;
    }
}

void chatAddUser(const char *channel, const char *name)
{
    char *prev, *newu;
    
    
    prev = hashSGet(dn_chats, channel);
    if (!prev) {
        prev = "";
    }
    
    newu = (char *) malloc((strlen(prev) + strlen(name) + 2) * sizeof(char));
    
    sprintf(newu, "%s%s\n", prev, name);
    
    hashSSet(dn_chats, channel, newu);
    
    free(newu);
    
}

void chatRemUser(const char *channel, const char *name)
{
    char *prev, *each[DN_MAX_PARAMS], *newu, *newp;
    int i, j;
    
    
    memset(each, 0, DN_MAX_PARAMS * sizeof(char *));
    
    prev = hashSGet(dn_chats, channel);
    if (prev) {
        prev = strdup(prev);
    } else {
        return;
    }
    
    newu = (char *) malloc((strlen(prev) + 1) * sizeof(char));
    newu[0] = '\0';
    newp = newu;
    
    /* split it */
    each[0] = prev;
    j = 1;
    for (i = 0; prev[i] != '\0'; i++) {
        if (prev[i] == '\n') {
            prev[i] = '\0';
            
            if (prev[i+1] != '\0') {
                each[j] = prev + i + 1;
                j++;
            }
        }
    }
    
    /* now recreate it */
    for (i = 0; each[i]; i++) {
        if (strncmp(each[i], name, strlen(name) + 1)) {
            strcpy(newp, each[i]);
            newp += strlen(each[i]);
            *newp = '\n';
            newp++;
        }
    }
    *newp = '\0';
    
    /* and put it back */
    hashSSet(dn_chats, channel, newu);
    
    free(newu);
    
}

char **chatUsers(const char *channel)
{
    char *prev, **each;
    int i, j;
    
    
    each = (char **) malloc(DN_MAX_PARAMS * sizeof(char *));
    
    memset(each, 0, DN_MAX_PARAMS * sizeof(char *));
    
    prev = hashSGet(dn_chats, channel);
    if (prev) {
        prev = strdup(prev);
    } else {
        prev = "";
    }
    
    /* split it */
    each[0] = prev;
    j = 1;
    for (i = 0; prev[i] != '\0'; i++) {
        if (prev[i] == '\n') {
            prev[i] = '\0';
            
            if (prev[i+1] != '\0') {
                each[j] = prev + i + 1;
                j++;
            }
        }
    }
    
    
    return each;
}

void chatJoin(const char *channel)
{
    if (!hashSGet(dn_chats, channel)) {
        hashSSet(dn_chats, channel, "");
    }
}

void chatLeave(const char *channel)
{
    hashSDelKey(dn_chats, channel);
}
