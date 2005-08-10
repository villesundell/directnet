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
#include "lock.h"

struct hashS *dn_chats;
DN_LOCK dn_chat_lock;

char chatOnChannel(char *channel)
{
    dn_lock(&dn_chat_lock);
    
    if (hashSGet(dn_chats, channel) != NULL) {
        dn_unlock(&dn_chat_lock);
        return 1;
    } else {
        dn_unlock(&dn_chat_lock);
        return 0;
    }
}

void chatAddUser(char *channel, char *name)
{
    char *prev, *new;
    
    dn_lock(&dn_chat_lock);
    
    prev = hashSGet(dn_chats, channel);
    if (!prev) {
        prev = "";
    }
    
    new = (char *) malloc((strlen(prev) + strlen(name) + 2) * sizeof(char));
    
    sprintf(new, "%s%s\n", prev, name);
    
    hashSSet(dn_chats, channel, new);
    
    free(new);
    
    dn_unlock(&dn_chat_lock);
}

void chatRemUser(char *channel, char *name)
{
    char *prev, *each[DN_MAX_PARAMS], *new, *newp;
    int i, j;
    
    dn_lock(&dn_chat_lock);
    
    memset(each, 0, DN_MAX_PARAMS * sizeof(char *));
    
    prev = hashSGet(dn_chats, channel);
    if (prev) {
        prev = strdup(prev);
    } else {
        return;
    }
    
    new = (char *) malloc((strlen(prev) + 1) * sizeof(char));
    new[0] = '\0';
    newp = new;
    
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
    hashSSet(dn_chats, channel, new);
    
    free(new);
    
    dn_unlock(&dn_chat_lock);
}

char **chatUsers(char *channel)
{
    char *prev, **each;
    int i, j;
    
    dn_lock(&dn_chat_lock);
    
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
    
    dn_unlock(&dn_chat_lock);
    
    return each;
}

void chatJoin(char *channel)
{
    dn_lock(&dn_chat_lock);
    if (!hashSGet(dn_chats, channel)) {
        hashSSet(dn_chats, channel, "");
    }
    dn_unlock(&dn_chat_lock);
}

void chatLeave(char *channel)
{
    dn_lock(&dn_chat_lock);
    hashSDelKey(dn_chats, channel);
    dn_unlock(&dn_chat_lock);
}
