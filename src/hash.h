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

#ifndef DN_HASH_H
#define DN_HASH_H

#include "lock.h"

#define DN_HASH_INTLIKE_H(htype, hshortn) \
struct hashKey##hshortn { \
    char key[24]; \
    htype value; \
}; \
struct hashKey##hshortn **hash##hshortn##Create(int count); \
void hash##hshortn##Destroy(struct hashKey##hshortn **hash); \
htype hash##hshortn##Get(struct hashKey##hshortn **hash, char *key); \
char *hash##hshortn##RevGet(struct hashKey##hshortn **hash, htype value); \
void hash##hshortn##Set(struct hashKey##hshortn **hash, char *key, htype value);

DN_HASH_INTLIKE_H(int, )
DN_HASH_INTLIKE_H(pthread_t, P)

struct hashKeyS {
    char key[24];
    char *value;
};

struct hashKeyL {
    char key[24];
    DN_LOCK value;
};

struct hashKeyS **hashSCreate(int count);
void hashSDestroy(struct hashKeyS **hash);
char *hashSGet(struct hashKeyS **hash, char *key);
char *hashSRevGet(struct hashKeyS **hash, char *value);
void hashSSet(struct hashKeyS **hash, char *key, char *value);
void hashSDelKey(struct hashKeyS **hash, char *key);

struct hashKeyL **hashLCreate(int count);
void hashLDestroy(struct hashKeyL **hash);
DN_LOCK *hashLGet(struct hashKeyL **hash, char *key);

/*struct hashKey {
    char key[24];
    int value;
};
struct hashKey **hashCreate(int count);
void hashDestroy(struct hashKey **hash);
int hashGet(struct hashKey **hash, char *key);
char *hashRevGet(struct hashKey **hash, int value);
void hashSet(struct hashKey **hash, char *key, int value);*/

#endif // DN_HASH_H
