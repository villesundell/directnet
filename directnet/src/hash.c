/*
 * Copyright 2004, 2005 Gregor Richards
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
struct hashKey##hshortn **hash##hshortn##Create(int count) \
{ \
    return (struct hashKey##hshortn **) malloc(count * sizeof(struct hashKey##hshortn *)); \
} \
\
void hash##hshortn##Destroy(struct hashKey##hshortn **hash) \
{ \
    int i; \
    \
    for (i = 0; hash[i] != NULL; i++) { \
        free(hash[i]); \
    } \
    \
    free(hash); \
} \
\
htype hash##hshortn##Get(struct hashKey##hshortn **hash, char *key) \
{ \
    int i; \
    \
    for (i = 0; hash[i] != NULL; i++) { \
        if (!strncmp(hash[i]->key, key, 25)) { \
            return hash[i]->value; \
        } \
    } \
    \
    return -1; \
} \
\
char *hash##hshortn##RevGet(struct hashKey##hshortn **hash, htype value) \
{ \
    int i; \
    \
    for (i = 0; hash[i] != NULL; i++) { \
        if (hash[i]->value == value) { \
            return hash[i]->key; \
        } \
    } \
    \
    return NULL; \
} \
\
void hash##hshortn##Set(struct hashKey##hshortn **hash, char *key, htype value) \
{ \
    int i; \
    \
    for (i = 0; hash[i] != NULL; i++) { \
        if (!strncmp(hash[i]->key, key, 25)) { \
            hash[i]->value = value; \
            return; \
        } \
    } \
    \
    hash[i] = (struct hashKey##hshortn *) malloc(sizeof(struct hashKey##hshortn)); \
    SF_strncpy(hash[i]->key, key, 24); \
    hash[i]->value = value; \
}

DN_HASH_INTLIKE_C(int, )
DN_HASH_INTLIKE_C(pthread_t, P)





// Then strings
struct hashKeyS **hashSCreate(int count)
{
    return (struct hashKeyS **) malloc(count * sizeof(struct hashKeyS *));
}

void hashSDestroy(struct hashKeyS **hash)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        free(hash[i]->value);
        free(hash[i]);
    }
    
    free(hash);
}

char *hashSGet(struct hashKeyS **hash, char *key)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        if (!strncmp(hash[i]->key, key, 25)) {
            return hash[i]->value;
        }
    }
    
    return NULL;
}

char *hashSRevGet(struct hashKeyS **hash, char *value)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        if (!strncmp(hash[i]->value, value, 25)) {
            return hash[i]->key;
        }
    }
    
    return NULL;
}

void hashSSet(struct hashKeyS **hash, char *key, char *value)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        if (!strncmp(hash[i]->key, key, 25)) {
            if (hash[i]->value != NULL) {
                free(hash[i]->value);
            }
            hash[i]->value = (char *) malloc((strlen(value) + 1) * sizeof(char));
            strcpy(hash[i]->value, value);
            return;
        }
    }
    
    hash[i] = (struct hashKeyS *) malloc(sizeof(struct hashKeyS));
    SF_strncpy(hash[i]->key, key, 24);
    hash[i]->value = (char *) malloc((strlen(value) + 1) * sizeof(char));
    strcpy(hash[i]->value, value);
}

void hashSDelKey(struct hashKeyS **hash, char *key)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        if (!strncmp(hash[i]->key, key, 25)) {
            if (hash[i]->value != NULL) {
                free(hash[i]->value);
            }
            hash[i]->value = NULL;
            return;
        }
    }
}





// Then locks
struct hashKeyL **hashLCreate(int count)
{
    return (struct hashKeyL **) malloc(count * sizeof(struct hashKeyL *));
}

void hashLDestroy(struct hashKeyL **hash)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        free(hash[i]);
    }
    
    free(hash);
}

DN_LOCK *hashLGet(struct hashKeyL **hash, char *key)
{
    int i;
    
    for (i = 0; hash[i] != NULL; i++) {
        if (!strncmp(hash[i]->key, key, 25)) {
            return &(hash[i]->value);
        }
    }
    
    // We have to create and initialize a lock for this
    hash[i] = (struct hashKeyL *) malloc(sizeof(struct hashKeyL));
    SF_strncpy(hash[i]->key, key, 24);
    dn_lockInit(&(hash[i]->value));
    
    return &(hash[i]->value);
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
