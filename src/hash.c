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

#include <stdlib.h>
#include <string.h>

#include "directnet.h"
#include "globals.h"
#include "hash.h"
#include "lock.h"

#define DN_HASH_INTLIKE_C(htype, hshortn) \
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
    struct hashKey##hshortn *cur = hash->head; \
    \
    while (cur) { \
        if (!strcmp(cur->key, key)) { \
            return cur->value; \
        } \
        cur = cur->next; \
    } \
    \
    return (htype) -1; \
} \
\
char *hash##hshortn##RevGet(struct hash##hshortn *hash, htype value) \
{ \
    struct hashKey##hshortn *cur = hash->head; \
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
    struct hashKey##hshortn *cur = hash->head; \
    struct hashKey##hshortn *toadd; \
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

DN_HASH_INTLIKE_C(int, I)
DN_HASH_INTLIKE_C(pthread_t *, P)





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
    struct hashKeyS *cur = hash->head;
    
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
    struct hashKeyS *cur = hash->head;
    
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
    struct hashKeyS *cur = hash->head;
    struct hashKeyS *toadd;
    
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
    struct hashKeyS *cur = hash->head;
    
    while (cur) {
        if (!strcmp(cur->key, key)) {
            if (cur->value) free(cur->value);
            cur->value = NULL;
            return;
        }
        cur = cur->next;
    }
}





// Then locks
struct hashL *hashLCreate()
{
    struct hashL *hash = (struct hashL *) malloc(sizeof(struct hashL));
    hash->head = NULL;
    return hash;
}

void hashLDestroy(struct hashL *hash)
{
    struct hashKeyL *cur = hash->head;
    struct hashKeyL *next;
    
    while (cur) {
        next = cur->next;
        free(cur->key);
        free(cur);
        cur = next;
    }
    
    free(hash);
}

DN_LOCK *hashLGet(struct hashL *hash, const char *key)
{
    struct hashKeyL *cur = hash->head;
    struct hashKeyL *toadd;
    
    while (cur) {
        if (!strcmp(cur->key, key)) {
            return &(cur->value);
        }
        cur = cur->next;
    }
    
    // We have to create and initialize a lock for this
    toadd = (struct hashKeyL *) malloc(sizeof(struct hashKeyL));
    toadd->key = strdup(key);
    dn_lockInit(&(toadd->value));
    toadd->next = NULL;
    
    if (hash->head) {
        cur = hash->head;
        while (cur->next) cur = cur->next;
        cur->next = toadd;
    } else {
        hash->head = toadd;
    }
    
    return &(toadd->value);
}

/*
// There are int versions and string versions, int first

struct hashKey **hashCreate(int count)
{
return (struct hashKey **) malloc(count * sizeof(struct hashKey *));
}

void hashDestroy(struct hashKey **hash)
{
int i;
    
for (i = 0; hash[i] != NULL; i++) {
free(hash[i]);
}
    
free(hash);
}

int hashGet(struct hashKey **hash, char *key)
{
int i;
    
for (i = 0; hash[i] != NULL; i++) {
if (!strncmp(hash[i]->key, key, 25)) {
return hash[i]->value;
}
}
    
return -1;
}

char *hashRevGet(struct hashKey **hash, int value)
{
int i;
    
for (i = 0; hash[i] != NULL; i++) {
if (hash[i]->value == value) {
return hash[i]->key;
}
}
    
return NULL;
}

void hashSet(struct hashKey **hash, char *key, int value)
{
int i;
    
for (i = 0; hash[i] != NULL; i++) {
if (!strncmp(hash[i]->key, key, 25)) {
hash[i]->value = value;
return;
}
}
    
hash[i] = (struct hashKey *) malloc(sizeof(struct hashKey));
strncpy(hash[i]->key, key, 25);
hash[i]->value = value;
}
*/
