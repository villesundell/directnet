/*
 * Copyright 2004 Gregor Richards
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
#include "hash.h"

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
    strncpy(hash[i]->key, key, 25);
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
