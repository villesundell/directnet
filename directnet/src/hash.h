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
    char *key; \
    htype value; \
    struct hashKey##hshortn *next; \
}; \
struct hash##hshortn { \
    struct hashKey##hshortn *head; \
}; \
struct hash##hshortn *hash##hshortn##Create(); \
void hash##hshortn##Destroy(struct hash##hshortn *hash); \
htype hash##hshortn##Get(struct hash##hshortn *hash, char *key); \
char *hash##hshortn##RevGet(struct hash##hshortn *hash, htype value); \
void hash##hshortn##Set(struct hash##hshortn *hash, char *key, htype value);

DN_HASH_INTLIKE_H(int, I)
DN_HASH_INTLIKE_H(pthread_t *, P)

struct hashKeyS {
    char *key;
    char *value;
    struct hashKeyS *next;
};

struct hashS {
    struct hashKeyS *head;
};

struct hashKeyL {
    char *key;
    DN_LOCK value;
    struct hashKeyL *next;
};

struct hashL {
    struct hashKeyL *head;
};

struct hashS *hashSCreate();
void hashSDestroy(struct hashS *hash);
char *hashSGet(struct hashS *hash, char *key);
char *hashSRevGet(struct hashS *hash, char *value);
void hashSSet(struct hashS *hash, char *key, char *value);
void hashSDelKey(struct hashS *hash, char *key);

struct hashL *hashLCreate();
void hashLDestroy(struct hashL *hash);
DN_LOCK *hashLGet(struct hashL *hash, char *key);

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
