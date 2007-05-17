/*
 * Copyright 2004, 2005, 2006  Gregor Richards
 * Many parts borrowed from cyfer/examples/pk.c
 * These parts are copyright 2004  Senko Rasic
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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "directnet/directnet.h"
#include "directnet/dnconfig.h"
#include "directnet/enc.h"
#include "directnet/message.h"
using namespace DirectNet;

#define bool BOOL

extern "C" {
#include "cyfer/cipher.h"
#include "cyfer/cyfer.h"
#include "cyfer/hash.h"
#include "cyfer/pk.h"
}

#define SYM_KEY_LEN 16
#define SYM_BLOCK_LEN 16

char encbinloc[256];

struct cyf_key {
    char *name;
    int keylen;
    char *key;
    struct cyf_key *next;
};

struct cyf_key *cyf_key_head = NULL;

char *mypukey, *myprkey;
size_t mypukeylen, myprkeylen;
BinSeq DirectNet::pukeyhash;

/* Helper function for creating public-key context. */
static CYFER_PK_CTX *init_pk_ctx()
{
    CYFER_PK_CTX *ctx;
    
    /* Initialize */
    ctx = CYFER_Pk_Init(CYFER_PK_RSA);
    if (!ctx) {
        perror("Error while generating keys");
        exit(EXIT_FAILURE);
    }
    return ctx;
}

/* Helper function for creating symmetric-cipher context. */
static CYFER_BLOCK_CIPHER_CTX *init_sym_ctx(unsigned char *keybuf, bool genkey)
{
    int i;
    CYFER_BLOCK_CIPHER_CTX *ctx;
    
    /* generate the key if asked to */
    if (genkey) {
        for (i = 0; i < SYM_KEY_LEN / sizeof(int); i++) {
            ((int *) keybuf)[i] = rand();
        }
    }
    
    /* Initialize */
    ctx = CYFER_BlockCipher_Init(CYFER_CIPHER_AES, keybuf, SYM_KEY_LEN, CYFER_MODE_CBC, NULL);
    if (!ctx) {
        perror("Error in creating encryption context");
    }
    return ctx;
}

/* Perform public-key encryption or decryption. */
char *pk_encdec(const char *name, const char *inp, int encrypt, int *len)
{
    char *key;
    int klen, loc, oloc, osl;
    size_t in_len, out_len;
    char *inbuf, *outbuf, *out;
    int import;
    CYFER_PK_CTX *ctx;
    struct cyf_key *cur;
    
    /* Create context */
    ctx = init_pk_ctx();
    
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

/* Perform symmetric-cipher encryption or decryption. */
char *sym_encdec(const char *inp, int encrypt, int *len, unsigned char *keybuf)
{
    int ilen, olen, i;
    char *out, *ibbuf, *obbuf;
    CYFER_BLOCK_CIPHER_CTX *ctx;
    
    /* Create context */
    if (encrypt) {
        ctx = init_sym_ctx(keybuf, true);
    } else {
        ctx = init_sym_ctx(keybuf, false);
    }
    if (!ctx) {
        return NULL;
    }
    
    /* Create the block buffers */
    ibbuf = (char *) malloc(SYM_BLOCK_LEN);
    obbuf = (char *) malloc(SYM_BLOCK_LEN);
    
    /* Create the output buffer */
    if (*len == 0) {
        ilen = strlen(inp);
    } else {
        ilen = *len;
    }
    
    /* When encrypting we include the modula-length (max 65k), when we're decrypting
       we can get the precise length easily */
    if (encrypt) {
        olen = ((ilen-1) / SYM_BLOCK_LEN + 1) * SYM_BLOCK_LEN;
        out = (char *) malloc(olen + 3);
        *len = olen + 2;
        intToCharray(ilen % SYM_BLOCK_LEN, out);
    } else {
        int modulalen;
        
        if (ilen < 2) {
            /* bad */
            free(ibbuf);
            free(obbuf);
            return NULL;
        }
        
        /* get the length */
        modulalen = charrayToInt(inp);
        if (modulalen == 0) modulalen = SYM_BLOCK_LEN;
        if (modulalen > SYM_BLOCK_LEN) {
            /* not right */
            free(ibbuf);
            free(obbuf);
            return NULL;
        }
        olen = ilen - (SYM_BLOCK_LEN - modulalen) - 2;
        if (olen < 0) {
            free(ibbuf);
            free(obbuf);
            return NULL;
        }
        
        /* allocate the output buffer */
        out = (char *) malloc(olen + 1);
        *len = olen;
        
        /* and pop off the length from input */
        inp += 2;
        ilen -= 2;
    }
    
    /* Encrypt or decrypt */
    for (i = 0; i < ilen; i += SYM_BLOCK_LEN) {
        /* set up this block */
        if (i + SYM_BLOCK_LEN > ilen) {
            memset(ibbuf, 0, SYM_BLOCK_LEN);
            memcpy(ibbuf, inp + i, ilen - i);
        } else {
            memcpy(ibbuf, inp + i, SYM_BLOCK_LEN);
        }
        
        /* encrypt or decrypt */
        if (encrypt) {
            CYFER_BlockCipher_Encrypt(ctx, (unsigned char *) ibbuf, (unsigned char *) obbuf);
        } else {
            CYFER_BlockCipher_Decrypt(ctx, (unsigned char *) ibbuf, (unsigned char *) obbuf);
        }
        
        /* copy into output */
        if (i + SYM_BLOCK_LEN > olen) {
            memcpy(out + i + (encrypt ? 2 : 0), obbuf, olen - i);
        } else {
            memcpy(out + i + (encrypt ? 2 : 0), obbuf, SYM_BLOCK_LEN);
        }
    }
    
    free(ibbuf);
    free(obbuf);
    CYFER_BlockCipher_Finish(ctx);
    
    return out;
}

int DirectNet::findEnc(char **envp)
{
    return 0;
}

BinSeq DirectNet::encTo(const BinSeq &from, const BinSeq &to, const BinSeq &msg)
{
    int len, emlen;
    char *encmsg, *enckey, *enckey2;
    unsigned char *keybuf;
    BinSeq ret;
    BinSeq msgVerified("DN");
    
    /* Make a message with a verifier segment */
    msgVerified += msg;
    
    /* First encrypt the message with a symmetric key */
    keybuf = (unsigned char *) malloc(SYM_KEY_LEN);
    len = msgVerified.size();
    encmsg = sym_encdec(msgVerified.c_str(), 1, &len, keybuf);
    if (!encmsg) {
        free(keybuf);
        return "";
    }
    emlen = len;
    
    /* Then encrypt and sign the symmetric key */
    len = SYM_KEY_LEN;
    enckey = pk_encdec(to.c_str(), (char *) keybuf, 1, &len); // encrypt
    enckey2 = pk_encdec(NULL, enckey, 1, &len); // sign
    free(enckey);
    free(keybuf);
    
    /* Add the keylength to the message (realistically always going to be the
       same, but doesn't hurt much) */
    ret.push_back("\0\0", 2);
    intToCharray(len, ret.c_str());
    
    /* Then put everything together */
    ret.push_back(enckey2, len);
    ret.push_back(encmsg, emlen);
    free(enckey2);
    free(encmsg);
    
    return ret;
}

BinSeq DirectNet::encFrom(const BinSeq &from, const BinSeq &to, const BinSeq &msg)
{
    int len, enckeylen;
    char *encmsg, *enckey, *enckey2;
    BinSeq ret;
    
    /* Sanity check */
    if (msg.size() < 2) {
        cout << "FAIL A" << endl;
        return "";
    }
    
    /* Get the size of the encrypted key */
    enckeylen = charrayToInt(msg.c_str());
    
    /* Sanity check #2 */
    if (msg.size() < enckeylen + 4) {
        cout << "FAIL B" << endl;
        return "";
    }
    
    /* Get the key, which is signed and encrypted */
    len = enckeylen;
    enckey = pk_encdec(from.c_str(), msg.c_str() + 2, 0, &len); // unsign
    enckey2 = pk_encdec(NULL, enckey, 0, &len); // decrypt
    free(enckey);
    
    /* Sanity check #3 */
    if (len < SYM_KEY_LEN) {
        cout << "FAIL C " << len << endl;
        return "";
    }
    
    /* Decrypt the message itself */
    len = msg.size() - (enckeylen + 2);
    encmsg = sym_encdec(msg.c_str() + enckeylen + 2, 0, &len, (unsigned char *) enckey2);
    free(enckey2);
    if (!encmsg) {
        cout << "FAIL D" << endl;
        return "";
    }
    
    /* Santicy check #4 */
    if (len < 2 ||
        encmsg[0] != 'D' ||
        encmsg[1] != 'N') {
        cout << "FAIL E" << endl;
        free(encmsg);
        return "";
    }
    
    /* Finally, the output message */
    ret.push_back(encmsg + 2, len - 2);
    free(encmsg);
    
    return ret;
}

// the file for the encryption key
string keyfile;

BinSeq genKey()
{
    CYFER_PK_CTX *ctx;
    
    /* Create context */
    ctx = init_pk_ctx();
    
    /* Generate and export keys into temporary buffers */
    CYFER_Pk_Generate_Key(ctx, 1024);
    CYFER_Pk_KeySize(ctx, &myprkeylen, &mypukeylen);
    
    myprkey = (char *) malloc(myprkeylen);
    if (!myprkey) { perror("malloc"); exit(1); }
    mypukey = (char *) malloc(mypukeylen);
    if (!mypukey) { perror("malloc"); exit(1); }
    
    CYFER_Pk_Export_Key(ctx, (unsigned char *) myprkey, (unsigned char *) mypukey);
    CYFER_Pk_Finish(ctx);
    
    pukeyhash = encHashKey(BinSeq(mypukey, mypukeylen));
    
    // put the key into the key file
    FILE *fkey = fopen(keyfile.c_str(), "w");
    if (fwrite(mypukey, 1, mypukeylen, fkey) != mypukeylen ||
        fwrite(myprkey, 1, myprkeylen, fkey) != myprkeylen) {
        // failed to write it, just remove it
        fclose(fkey);
        unlink(keyfile.c_str()); // ignore errors from this
        
    } else {
        // wrote the data, now chmod it if applicable
        fclose(fkey);
#ifndef __WIN32
        chmod(keyfile.c_str(), 0600); // ignore errors (can't do much about it)
#endif
    }
    
    return BinSeq(mypukey, mypukeylen);
}

BinSeq DirectNet::encCreateKey()
{
    srand(time(NULL));
    keyfile = cfgdir + "/key-" + genericName(dn_name).c_str();
    
    if (access(keyfile.c_str(), R_OK) != 0) {
        // can't read the file
        return genKey();
    }
    
    // already have an encryption key, just read it in
    FILE *fkey = fopen(keyfile.c_str(), "rb");
    if (!fkey) {
        return genKey();
    }
    
    // FIXME: these sizes should be detected
    myprkeylen = 260;
    mypukeylen = 260;
        
    /* allocate space */
    myprkey = (char *) malloc(myprkeylen);
    if (!myprkey) { perror("malloc"); exit(1); }
    mypukey = (char *) malloc(mypukeylen);
    if (!mypukey) { perror("malloc"); exit(1); }
    
    // OK, have a key file, read in the keys
    if (fread(mypukey, 1, mypukeylen, fkey) != mypukeylen ||
        fread(myprkey, 1, myprkeylen, fkey) != myprkeylen) {
        // failed to read the key
        free(myprkey);
        free(mypukey);
        return genKey();
    }
    
    pukeyhash = encHashKey(BinSeq(mypukey, mypukeylen));
    
    return BinSeq(mypukey, mypukeylen);
}

BinSeq DirectNet::encExportKey() {
    return BinSeq(mypukey, mypukeylen);
}

BinSeq DirectNet::encHashKey(const BinSeq &key)
{
    BinSeq tr("                                ");
    CYFER_Hash(CYFER_HASH_SHA256, (unsigned char *) key.c_str(), key.size(),
               (unsigned char *) tr.c_str());
    return tr;
}

BinSeq DirectNet::encImportKey(const BinSeq &name, const BinSeq &key)
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

BinSeq DirectNet::encSub(const BinSeq &a, const BinSeq &b, int *remainder)
{
    BinSeq r, rrev;
    if (a.size() != b.size()) return r;
    int carry = 0;
    int ca, cb;
    
    for (int i = a.size() - 1; i >= 0; i--) {
        ca = (unsigned char) a[i];
        cb = (unsigned char) b[i];
        ca -= carry;
        carry = 0;
        
        while (cb > ca) {
            carry++;
            cb -= 256;
        }
        ca -= cb;
        rrev.push_back((unsigned char) ca);
    }
    for (int i = rrev.size() - 1; i >= 0; i--) {
        r.push_back(rrev[i]);
    }
    
    if (remainder) *remainder = carry;
    
    return r;
}

BinSeq DirectNet::encAdd(const BinSeq &a, const BinSeq &b, int *remainder)
{
    BinSeq r, rrev;
    if (a.size() != b.size()) return r;
    int carry = 0;
    int ca, cb;
    
    for (int i = a.size() - 1; i >= 0; i--) {
        ca = (unsigned char) a[i];
        cb = (unsigned char) b[i];
        ca += cb + carry;
        carry = 0;
        
        while (ca >= 256) {
            carry++;
            ca -= 256;
        }
        rrev.push_back((unsigned char) ca);
    }
    for (int i = rrev.size() - 1; i >= 0; i--) {
        r.push_back(rrev[i]);
    }
    
    if (remainder) *remainder = carry;
    
    return r;
}

int DirectNet::encCmp(const BinSeq &a, const BinSeq &b)
{
    BinSeq ra, rb, rs;
    ra = encSub(a, pukeyhash);
    rb = encSub(b, pukeyhash);
    
    // subtract with remainder to compare
    int r;
    rs = encSub(ra, rb, &r);
    if (r) {
        return -1; // if anything remaining, rb was greater than ra
    } else {
        // if it's empty, they're equal
        for (int i = 0; i < rs.size(); i++) {
            if (rs[i] != '\0') return 1;
        }
        return 0;
    }
}

#undef bool

bool DirectNet::encCloser(const BinSeq &a, const BinSeq &b)
{
    BinSeq meb;
    meb = encSub(b, pukeyhash);
    if (meb[0] & 0x80) {
        // negative value isn't useful here
        meb = encSub(pukeyhash, b);
    }
    
    BinSeq ab;
    ab = encSub(b, a);
    if (ab[0] & 0x80) {
        ab = encSub(a, b);
    }
    
    // now just compare these two differences
    BinSeq res;
    int rem;
    res = encSub(meb, ab, &rem);
    if (rem) {
        // ab is bigger, so I'm closer
        return false;
    }
    return true;
}

BinSeq DirectNet::encOffset(int by, bool reverse)
{
    BinSeq a;
    int hi, lo;
    
    // a is the key to add, generate it
    for (int i = 0; i < pukeyhash.size(); i++) a.push_back((char) 0);
    
    hi = by >> 3;
    lo = 7 - (by & 0x7);
    a[hi] = ((unsigned char) 0x1) << lo;
    
    // add or subtract them
    if (!reverse) {
        return encAdd(pukeyhash, a);
    } else {
        return encSub(pukeyhash, a);
    }
}
