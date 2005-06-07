/*
 * Copyright 2004, 2005 Gregor Richards
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

#include <stdio.h>

#include "cyfer/pk.h"

#include "directnet.h"

char gpghomedir[256], gpgbinloc[256];

struct cyf_key {
    char *name;
    char *key;
    struct cyf_key *next;
};

struct cyf_key *cyf_key_head = NULL;

char *mypukey, *myprkey;

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

/* our keys need to be in hex */
char *mkhex(int len, unsigned char *inp)
{
    int i;
    unsigned char hi, lo;
    
    char *out;
    out = (char *) malloc(((len * 2) + 1) * sizeof(char));
    
    for (i = 0; i < len; i++) {
        lo = inp[i] & 15;
        hi = (inp[i] & 240) >> 4;
        
        if (hi < 10) {
            out[i * 2] = '0' + hi;
        } else {
            out[i * 2] = 'A' + hi - 10;
        }
        
        if (lo < 10) {
            out[(i * 2) + 1] = '0' + lo;
        } else {
            out[(i * 2) + 1] = 'A' + lo - 10;
        }
    }
    out[i << 1] = '\0';
    
    return out;
}

char *dehex(int *len, unsigned char *inp)
{
    int i, osl, ol;
    
    unsigned char *out;
    
    osl = strlen(inp);
    *len = osl / 2;
    out = (char *) malloc((*len + 1) * sizeof(char));
    out[*len] = '\0';
    
    for (i = 0; i < osl; i += 2) {
        ol = i >> 1;
        out[ol] = 0;

        if (inp[i] >= '0' && inp[i] <= '9') {
            out[ol] += (inp[i] - '0') << 4;
        } else {
            out[ol] += (inp[i] - 'A' + 10) << 4;
        }
        
        if (inp[i+1] >= '0' && inp[i+1] <= '9') {
            out[ol] += inp[i+1] - '0';
        } else {
            out[ol] += inp[i+1] - 'A' + 10;
        }
    }
    ol = i >> 1;
    out[ol] = '\0';
    
    return out;
}

/* Perform encryption or decryption. */
char *encdec(char *pk, char *name, char *inp, int encrypt, int *len)
{
    char *key;
    int klen, loc, oloc, osl;
    size_t in_len, out_len;
    char *inbuf, *outbuf, *out, *outh;
    int import;
    int n;
    CYFER_PK_CTX *ctx;
    struct cyf_key *cur;
    
    /* Create context */
    ctx = init_ctx(pk);

    if (name != NULL) {
        cur = cyf_key_head;
        while (cur) {
            if (!strcmp(cur->name, name)) {
                key = dehex(&klen, cur->key);
                break;
            }
        }
        if (!cur) return strdup("");
    } else {
        key = dehex(&klen, myprkey);
    }
    
    if (encrypt) {
        import = CYFER_Pk_Import_Key(ctx, NULL, 0, key, klen);
        CYFER_Pk_Size(ctx, &in_len, &out_len);
    } else {
        import = CYFER_Pk_Import_Key(ctx, key, klen, NULL, 0);
        CYFER_Pk_Size(ctx, &out_len, &in_len);
    }

    if (!import) {
        return strdup("");
    }

    /* Create data buffers */
    inbuf = malloc(in_len);
    outbuf = malloc(out_len);
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
            CYFER_Pk_Encrypt(ctx, inbuf, outbuf);
        else
            CYFER_Pk_Decrypt(ctx, inbuf, outbuf);
        
        memcpy(out + oloc, outbuf, out_len);
        
        oloc += out_len;
    }

    CYFER_Pk_Finish(ctx);
    free(key); free(inbuf); free(outbuf);
    
    *len = oloc;
    return out;
}

int findGPG(char **envp)
{
    return 0;
}
    
char *gpgTo(char *from, char *to, char *msg)
{
    int len;
    char *enc, *enc2;
    
    len = 0;
    enc = encdec("RSA", to, msg, 1, &len); // encrypt
    enc2 = encdec("RSA", NULL, enc, 1, &len); // sign
    free(enc);
    
    enc = mkhex(len, enc2); // hex
    free(enc2);
    
    return enc;
}

char *gpgFrom(char *from, char *to, char *msg)
{
    int len;
    char *enc, *enc2;
    
    enc = dehex(&len, msg); // dehex
    
    enc2 = encdec("RSA", from, enc, 0, &len); // de-sign
    free(enc);
    
    enc = encdec("RSA", NULL, enc2, 0, &len); // decrypt
    free(enc2);
    
    return enc;
}

char *gpgCreateKey()
{
    CYFER_PK_CTX *ctx;
    int privlen, publen;
    char *priv, *pub;

    /* Create context */
    ctx = init_ctx("RSA");
    
    /* Generate and export keys into temporary buffers */
    CYFER_Pk_Generate_Key(ctx, 1024);
    CYFER_Pk_KeySize(ctx, &privlen, &publen);

    priv = malloc(privlen);
    pub = malloc(publen);

    CYFER_Pk_Export_Key(ctx, priv, pub);
    CYFER_Pk_Finish(ctx);
    
    myprkey = mkhex(privlen, priv);
    mypukey = mkhex(publen, pub);
    
    free(priv); free(pub);
    
    return mypukey;
}

char *gpgExportKey() {
    return mypukey;
}

char *gpgImportKey(char *name, char *key) {
    if (cyf_key_head) {
        struct cyf_key *cur = cyf_key_head;
        while (cur->next) {
            if (!strcmp(cur->name, name)) {
                cur->key = strdup(key);
                return "";
            }
            cur = cur->next;
        }
        
        cur->next = (struct cyf_key *) malloc(sizeof(struct cyf_key));
        cur->next->name = strdup(name);
        cur->next->key = strdup(key);
        cur->next->next = NULL;
    } else {
        cyf_key_head = (struct cyf_key *) malloc(sizeof(struct cyf_key));
        cyf_key_head->name = strdup(name);
        cyf_key_head->key = strdup(key);
        cyf_key_head->next = NULL;
    }
    
    return "";
}
