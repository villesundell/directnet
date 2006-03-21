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

#if 0

#include <stdlib.h>
#include <string.h>

#include "directnet.h"
#include "globals.h"
#include "hash.h"

#define DN_HASH_INTLIKE_C(htype, hshortn, defval) \
struct hash##hshortn *hash##hshortn##Create() \
{ \
    struct hash##hshortn *hash = (struct hash##hshortn *) malloc(sizeof(struct hash##hshortn)); \
    hash->head = NULL; \
    return hash; \
} \
\
void hash##hshortn##Destroy(struct hash##hshortn *hash) \
{ \
    struct hashKey##hshortn *cur = hash->head; \
    struct hashKey##hshortn *next; \
    \
    while (cur) { \
        next = cur->next; \
        free(cur->key); \
        free(cur); \
        cur = next; \
    } \
    \
    free(hash); \
} \
\
htype hash##hshortn##Get(struct hash##hshortn *hash, const char *key) \
{ \
    struct hashKey##hshortn *cur; \
    \
    cur = hash->head; \
    \
    while (cur) { \
        if (!strcmp(cur->key, key)) { \
            return cur->value; \
        } \
        cur = cur->next; \
    } \
    \
    return (defval); \
} \
\
char *hash##hshortn##RevGet(struct hash##hshortn *hash, htype value) \
{ \
    struct hashKey##hshortn *cur; \
    \
    cur = hash->head; \
    \
    while (cur) { \
        if (cur->value == value) { \
            return cur->key; \
        } \
        cur = cur->next; \
    } \
    \
    return NULL; \
} \
\
void hash##hshortn##Set(struct hash##hshortn *hash, const char *key, htype value) \
{ \
    struct hashKey##hshortn *cur; \
    struct hashKey##hshortn *toadd; \
    \
    cur = hash->head; \
    \
    while (cur) { \
        if (!strcmp(cur->key, key)) { \
            cur->value = value; \
            return; \
        } \
        cur = cur->next; \
    } \
    \
    toadd = (struct hashKey##hshortn *) malloc(sizeof(struct hashKey##hshortn)); \
    toadd->key = strdup(key); \
    toadd->value = value; \
    toadd->next = NULL; \
    if (hash->head) { \
        cur = hash->head; \
        while (cur->next) cur = cur->next; \
        cur->next = toadd; \
    } else { \
        hash->head = toadd; \
    } \
}

DN_HASH_INTLIKE_C(int, I, -1)
DN_HASH_INTLIKE_C(void *, V, NULL)





// Then strings
struct hashS *hashSCreate()
{
    struct hashS *hash = (struct hashS *) malloc(sizeof(struct hashS));
    hash->head = NULL;
    return hash;
}

void hashSDestroy(struct hashS *hash)
{
    struct hashKeyS *cur = hash->head;
    struct hashKeyS *next;
    
    while (cur) {
        next = cur->next;
        free(cur->key);
        if (cur->value) free(cur->value);
        free(cur);
        cur = next;
    }
    
    free(hash);
}

char *hashSGet(struct hashS *hash, const char *key)
{
    struct hashKeyS *cur;
    
    cur = hash->head;
    
    while (cur) {
        if (!strcmp(cur->key, key)) {
            return cur->value;
        }
        cur = cur->next;
    }
    
    return NULL;
}

char *hashSRevGet(struct hashS *hash, const char *value)
{
    struct hashKeyS *cur;
    
    cur = hash->head;
    
    while (cur) {
        if (!strcmp(cur->value, value)) {
            return cur->key;
        }
        cur = cur->next;
    }
    
    return NULL;
}

void hashSSet(struct hashS *hash, const char *key, const char *value)
{
    struct hashKeyS *cur;
    struct hashKeyS *toadd;
    
    cur = hash->head;
    
    while (cur) {
        if (!strcmp(cur->key, key)) {
            if (cur->value) free(cur->value);
            cur->value = strdup(value);
            return;
        }
        cur = cur->next;
    }
    
    toadd = (struct hashKeyS *) malloc(sizeof(struct hashKeyS));
    toadd->key = strdup(key);
    toadd->value = strdup(value);
    toadd->next = NULL;
    
    if (hash->head) {
        cur = hash->head;
        while (cur->next) cur = cur->next;
        cur->next = toadd;
    } else {
        hash->head = toadd;
    }
}

void hashSDelKey(struct hashS *hash, const char *key)
{
    struct hashKeyS *cur;
    
    cur = hash->head;
    
    while (cur) {
        if (!strcmp(cur->key, key)) {
            if (cur->value) free(cur->value);
            cur->value = NULL;
            return;
        }
        cur = cur->next;
    }
}

#endif
