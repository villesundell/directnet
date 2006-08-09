/*
 * Copyright 2004, 2005, 2006 Gregor Richards
 * Many parts borrowed from cyfer/examples/pk.c
 * These parts are copyright 2004 Senko Rasic
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

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directnet.h"
#include "enc.h"

#define bool BOOL

extern "C" {
#include "cyfer/hash.h"
#include "cyfer/pk.h"
}

char encbinloc[256];

struct cyf_key {
    char *name;
    int keylen;
    char *key;
    struct cyf_key *next;
};

struct cyf_key *cyf_key_head = NULL;

char *mypukey, *myprkey;
int mypukeylen, myprkeylen;
BinSeq pukeyhash;

/* Helper function for creating public-key context. */
static CYFER_PK_CTX *init_ctx(char *pk)
{
    int type;
    bool enc, sig;
    CYFER_PK_CTX *ctx;

    /* Select and initialize the desired algorithm or fail miserably */
    type = CYFER_Pk_Select(pk, &enc, &sig);
    if (type == CYFER_PK_NONE) {
        return NULL;
    }
    ctx = CYFER_Pk_Init(type);
    if (!ctx) {
        perror("Error while generating keys");
        exit(EXIT_FAILURE);
    }
    return ctx;
}

/* Perform encryption or decryption. */
char *encdec(char *pk, const char *name, const char *inp, int encrypt, int *len)
{
    char *key;
    int klen, loc, oloc, osl;
    size_t in_len, out_len;
    char *inbuf, *outbuf, *out;
    int import;
    CYFER_PK_CTX *ctx;
    struct cyf_key *cur;
    
    /* Create context */
    ctx = init_ctx(pk);

    if (name != NULL) {
        cur = cyf_key_head;
        while (cur) {
            if (!strcmp(cur->name, name)) {
                klen = cur->keylen;
                key = cur->key;
                break;
            }
            cur = cur->next;
        }
        if (!cur) return strdup("");
    } else {
        klen = myprkeylen;
        key = myprkey;
    }
    
    if (encrypt) {
        import = CYFER_Pk_Import_Key(ctx, NULL, 0, (unsigned char *) key, klen);
        CYFER_Pk_Size(ctx, &in_len, &out_len);
    } else {
        import = CYFER_Pk_Import_Key(ctx, (unsigned char *) key, klen, NULL, 0);
        CYFER_Pk_Size(ctx, &out_len, &in_len);
    }

    if (!import) {
        return strdup("");
    }

    /* Create data buffers */
    inbuf = (char *) malloc(in_len);
    outbuf = (char *) malloc(out_len);
    out = NULL;
    loc = 0;
    oloc = 0;

    /* Encrypt or decrypt. For incomplete block, this assumes the remainder
     * is filled with zeroes.  */
    if (*len == 0) {
        osl = strlen(inp);
    } else {
        osl = *len;
    }
    for (loc = 0; loc < osl; loc += in_len) {
        memcpy(inbuf, inp + loc, in_len);
        out = (char *) realloc(out, oloc + out_len);
        
        if (encrypt)
            CYFER_Pk_Encrypt(ctx, (unsigned char *) inbuf, (unsigned char *) outbuf);
        else
            CYFER_Pk_Decrypt(ctx, (unsigned char *) inbuf, (unsigned char *) outbuf);
        
        memcpy(out + oloc, outbuf, out_len);
        
        oloc += out_len;
    }

    CYFER_Pk_Finish(ctx);
    free(inbuf); free(outbuf);
    
    *len = oloc;
    return out ? out : strdup("");
}

int findEnc(char **envp)
{
    return 0;
}

BinSeq encTo(const BinSeq &from, const BinSeq &to, const BinSeq &msg)
{
    int len;
    char *enc, *enc2;
    BinSeq ret;
    
    len = msg.size();
    enc = encdec("RSA", to.c_str(), msg.c_str(), 1, &len); // encrypt
    enc2 = encdec("RSA", NULL, enc, 1, &len); // sign
    free(enc);
    
    ret.push_back(enc2, len);
    free(enc2);
    
    return ret;
}

BinSeq encFrom(const BinSeq &from, const BinSeq &to, const BinSeq &msg)
{
    int len;
    char *enc, *enc2;
    BinSeq ret;
    
    len = msg.size();
    enc = encdec("RSA", from.c_str(), msg.c_str(), 0, &len); // de-sign
    enc2 = encdec("RSA", NULL, enc, 0, &len); // decrypt
    free(enc);
    
    ret.push_back(enc2, len);
    free(enc2);
    
    return ret;
}

BinSeq encCreateKey()
{
    CYFER_PK_CTX *ctx;
    
    /* Create context */
    ctx = init_ctx("RSA");
    
    /* Generate and export keys into temporary buffers */
    srand(time(NULL));
    CYFER_Pk_Generate_Key(ctx, 1024);
    CYFER_Pk_KeySize(ctx, (unsigned int *) &myprkeylen, (unsigned int *) &mypukeylen);
    
    myprkey = (char *) malloc(myprkeylen);
    if (!myprkey) { perror("malloc"); exit(1); }
    mypukey = (char *) malloc(mypukeylen);
    if (!mypukey) { perror("malloc"); exit(1); }
    
    CYFER_Pk_Export_Key(ctx, (unsigned char *) myprkey, (unsigned char *) mypukey);
    CYFER_Pk_Finish(ctx);
    
    pukeyhash = encHashKey(BinSeq(mypukey, mypukeylen));
    
    return BinSeq(mypukey, mypukeylen);
}

BinSeq encExportKey() {
    return BinSeq(mypukey, mypukeylen);
}

BinSeq encHashKey(const BinSeq &key)
{
    BinSeq tr("                                ");
    CYFER_Hash(CYFER_HASH_SHA256, (unsigned char *) key.c_str(), key.size(),
               (unsigned char *) tr.c_str());
    return tr;
}

BinSeq encImportKey(const BinSeq &name, const BinSeq &key)
{
    if (cyf_key_head) {
        struct cyf_key *cur = cyf_key_head;
        
        while (cur->next) {
            if (!strcmp(cur->name, name.c_str())) {
                free(cur->key);
                
                cur->keylen = key.size();
                
                cur->key = (char *) malloc(key.size());
                if (cur->key == NULL) { perror("malloc"); exit(1); }
                memcpy(cur->key, key.c_str(), key.size());
                
                return BinSeq();
            }
            cur = cur->next;
        }
        if (!strcmp(cur->name, name.c_str())) {
            free(cur->key);
            
            cur->keylen = key.size();
            
            cur->key = (char *) malloc(key.size());
            if (cur->key == NULL) { perror("malloc"); exit(1); }
            memcpy(cur->key, key.c_str(), key.size());
            
            return BinSeq();
        }
        
        cur->next = (struct cyf_key *) malloc(sizeof(struct cyf_key));
        cur->next->name = strdup(name.c_str());
        
        cur->next->keylen = key.size();
        
        cur->next->key = (char *) malloc(key.size());
        if (cur->next->key == NULL) { perror("malloc"); exit(1); }
        memcpy(cur->next->key, key.c_str(), key.size());
        
        cur->next->next = NULL;
    } else {
        cyf_key_head = (struct cyf_key *) malloc(sizeof(struct cyf_key));
        cyf_key_head->name = strdup(name.c_str());
        
        cyf_key_head->keylen = key.size();
        
        cyf_key_head->key = (char *) malloc(key.size());
        if (cyf_key_head->key == NULL) { perror("malloc"); exit(1); }
        memcpy(cyf_key_head->key, key.c_str(), key.size());
        
        cyf_key_head->next = NULL;
    }
    return BinSeq();
}

BinSeq encSub(const BinSeq &a, const BinSeq &b, int *remainder)
{
    BinSeq r;
    if (a.size() != b.size()) return r;
    int carry = 0;
    int ca, cb;
    
    for (int i = 0; i < a.size(); i++) {
        ca = (unsigned char) a[i];
        cb = (unsigned char) b[i];
        cb += carry;
        carry = 0;
        
        while (cb > ca) {
            carry++;
            cb -= 256;
        }
        ca -= cb;
        r.push_back((unsigned char) ca);
    }
    
    if (remainder) *remainder = carry;
}

BinSeq encAdd(const BinSeq &a, const BinSeq &b, int *remainder)
{
    BinSeq r;
    if (a.size() != b.size()) return r;
    int carry = 0;
    int ca, cb;
    
    for (int i = 0; i < a.size(); i++) {
        ca = (unsigned char) a[i];
        cb = (unsigned char) b[i];
        ca += cb + carry;
        carry = 0;
        
        while (ca > 255) {
            carry++;
            ca -= 256;
        }
        r.push_back((unsigned char) ca);
    }
    
    if (remainder) *remainder = carry;
}

int encCmp(const BinSeq &a, const BinSeq &b)
{
    BinSeq ra, rb, rs;
    ra = encSub(pukeyhash, a);
    rb = encSub(pukeyhash, b);
    
    // subtract with remainder to compare
    int r;
    rs = encSub(ra, rb, &r);
    if (r) {
        return 1; // if anything remaining, rb was greater than ra
    } else {
        // if it's empty, they're equal
        for (int i = 0; i < rs.size(); i++) {
            if (rs[i] != '\0') return -1;
        }
        return 0;
    }
}

/* Generate a key offset from yours by 1/(2^by)
 * returns: the generated key */
BinSeq encOffset(int by)
{
    BinSeq a;
    int hi, lo;
    
    // a is the key to add, generate it
    for (int i = 0; i < mypukeylen; i++) a.push_back((char) 0);
    
    hi = by >> 3;
    lo = by & 0x7;
    a[hi] = ((unsigned char) 0x1) << lo;
    
    // add them
    return encAdd(BinSeq(mypukey, mypukeylen), a);
}
