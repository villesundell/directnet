/*
 * Copyright 2005  Gregor Richards
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "auth.h"
#include "directnet.h"

/* MingW32 pipe &c: */
#ifdef _WIN32
#include <fcntl.h>
#define pipe(a) _pipe((a), 0, _O_BINARY | _O_NOINHERIT)

#ifndef WEXITSTATUS
#define WEXITSTATUS(s)  ((s) & 0xff)
#endif

#else
#include <sys/wait.h>
#endif

int GPG_have;
char *GPG_bin = NULL;
char *GPG_name = NULL;
char *GPG_pass = NULL;

/* Username and pass strings */
char authUsername[] = "GPG Username (blank for none)";
char authPW[] = "GPG Password";

#define CPASS if (!GPG_name || !GPG_pass || !GPG_have) 

/* Wrap for GPG
 * inp: input to GPG
 * args: arguments to pass to GPG
 * pass: 1 if it needs a passphrase
 * returns: a MALLOC'D buffer with the output */
char *gpgWrap(const char *inp, const char *args, int pass)
{
    FILE *fi, *fo;
    char *co;
    int pspp[2], res, pid;
    char *cmd, *pspc;
    
    /* 1) open files */
    fi = tmpfile();
    if (!fi) { perror("tmpfile"); return NULL; }
    fo = tmpfile();
    if (!fo) { perror("tmpfile"); return NULL; }
    
    /* 2) write our input */
    fputs(inp, fi);
    fseek(fi, 0, SEEK_SET);
    
    /* 3) pipe the passphrase */
    pspc = strdup(" ");
    if (pass) {
        if (pipe(pspp) == -1) {
            perror("pipe");
            return NULL;
        }
        write(pspp[1], GPG_pass, strlen(GPG_pass));
        close(pspp[1]);
        
        /* pspc contains the --passphrase-fd arg */
        free(pspc);
        pspc = (char *) malloc(25 * sizeof(char));
        sprintf(pspc, "--passphrase-fd %d", pspp[0]);
    }
    
    /* 4) fork and run the command */
    pid = fork();
    if (pid == 0) {
        cmd = (char *) malloc((strlen(GPG_bin) + strlen(args) + strlen(pspc) + 12) * sizeof(char));
        if (cmd == NULL) { exit(-1); }
        sprintf(cmd, "%s %s %s --no-tty", GPG_bin, args, pspc);
        
        dup2(fileno(fi), 0);
        dup2(fileno(fo), 1);
        res = open("/dev/null", O_WRONLY);
        if (res > -1) {
            dup2(res, 2);
        }
        
        execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
        exit(1);
    } else if (pid > 0) {
        /* wait for the child */
        waitpid(pid, &res, 0);
    } else if (pid < 0) {
        perror("fork");
        return NULL;
    }
    
    if (WEXITSTATUS(res) == 127) return NULL;
    
    /* 5) extract the content of fo */
    fseek(fo, 0, SEEK_SET);
    co = (char *) malloc(10240 * sizeof(char));
    co[fread(co, 1, 10240, fo)] = '\0';
    
    fclose(fi);
    fclose(fo);
    free(pspc);
    return co;
}

int authInit()
{
    char *ret;
    int i;
    
    GPG_have = 0;
    
    // try a number of possible paths
    for (i = 0; i < 3; i++) {
        switch (i) {
            case 0:
                // try no path first
                GPG_bin = strdup("gpg");
                if (!GPG_bin) { perror("strdup"); exit(1); }
                ret = gpgWrap("", "--version", 0);
                break;
                
            case 1:
                // now try with bindir
                GPG_bin = (char *) malloc(strlen(bindir) + 5);
                if (!GPG_bin) { perror("malloc"); exit(1); }
                
                sprintf(GPG_bin, "%s/gpg", bindir);
                ret = gpgWrap("", "--version", 0);
                break;
                
            case 2:
                // now try with /sw/bin (for Fink)
                GPG_bin = strdup("/sw/bin/gpg");
                if (!GPG_bin) { perror("strdup"); exit(1); }
                ret = gpgWrap("", "--version", 0);
                break;
        }
        
        if (ret == NULL) {
            // nope!
            free(GPG_bin);
        } else {
            // this is it
            GPG_have = 1;
            free(ret);
            break;
        }
    }
    
    GPG_name = NULL;
    GPG_pass = NULL;
    return 1;
}

int authNeedPW()
{
    return 1;
}

void authSetPW(const char *nm, const char *pswd)
{
    if (nm[0]) {
        GPG_name = strdup(nm);
        GPG_pass = strdup(pswd);
    }
}

char *authSign(const char *msg)
{
    char *arg, *outp;
    
    CPASS return strdup(msg);
    
    arg = (char *) malloc((18 + strlen(GPG_name)) * sizeof(char));
    sprintf(arg, "--clearsign -u \"%s\"", GPG_name);
    
    outp = gpgWrap(msg, arg, 1);
    free(arg);
    return outp ? outp : strdup(msg);
}

char *authVerify(const char *msg, char **who, int *status)
{
    char *toret, *validity, *tmp, *tmp2;
    
    *who = NULL;
    *status = -1;
    
    /* the special case is that this is a key */
    if (!strncmp(msg, "-----BEGIN PGP PUBLIC KEY BLOCK-----\n", 37)) {
        CPASS return strdup("");
        /* figure out who */
        if ((validity = gpgWrap(msg, "", 0))) {
            /* look for "pub" */
            char *curl = validity;
            while (curl) {
                if (!strncmp(curl, "pub  ", 5)) {
                    /* Look for the name, after two ' 's */
                    curl += 5;
                    curl = strchr(curl, ' ');
                    if (curl) {
                        curl = strchr(++curl, ' ');
                        if (curl) {
                            /* This seems valid! */
                            tmp = strchr(++curl, '\n');
                            if (tmp) {
                                /* Now we have the whole string */
                                *tmp = '\0';
                                *who = strdup(curl);
                                *status = 2;
                                *tmp = '\n';
                            }
                        }
                    }
                    curl = NULL;
                } else {
                    curl = strchr(curl, '\n');
                    if (curl) curl++;
                }
            }
            
            free(validity);
        }
        return NULL;
    }
    
    /* sanity check */
    if (strncmp(msg, "-----BEGIN PGP SIGNED MESSAGE-----\n", 35)) {
        return strdup(msg);
    }
    
    /* first test the validity */
    CPASS goto getmsg;
    if ((validity = gpgWrap(msg, "--verify --status-fd 1", 0))) {
        char *curl = validity;
        while (curl) {
            if (!strncmp(curl, "[GNUPG:] GOODSIG ", 17)) {
                *status = 1;
                
                /* find the name */
                curl += 17;
                tmp = strchr(curl, ' ');
                if (tmp) {
                    tmp++;
                    
                    /* now we're at the beginning of the name */
                    curl = tmp;
                    tmp = strchr(curl, '\n');
                    tmp2 = strchr(curl, '<');
                    if (tmp2) tmp2--;
                    if (tmp2 < tmp) tmp = tmp2;
                    if (tmp) {
                        /* now we're at the end */
                        *tmp = '\0';
                        *who = strdup(curl);
                        *tmp = '\n';
                    }
                }
                
                /* make sure it doesn't continue looping */
                curl = NULL;
            } else if (!strncmp(curl, "[GNUPG:] ERRSIG ", 16)) {
                *status = 0;
                curl = NULL;
            } else if (!strncmp(curl, "[GNUPG:] NODATA ", 16)) {
                *status = 0;
                curl = NULL;
            }
            
            if (curl) {
                curl = strchr(curl, '\n');
                if (curl) curl++;
            }
        }
        
        free(validity);
    }
    
    /* just grab out the message portion */
    /* look for the first blank line */
    char *curl;
    getmsg:
    toret = NULL;
    curl = strchr(msg, '\n');
    while (curl) {
        curl++;
        if (*curl == '\n') {
            /* this is the blank line */
            char *cure;
            curl++;
            cure = strchr(curl, '\n');
            while (cure) {
                if (!strncmp(cure + 1, "-----BEGIN PGP SIGNATURE-----\n", 30)) {
                    /* strdup what we have */
                    *cure = '\0';
                    toret = strdup(curl);
                    *cure = '\n';
                    cure = NULL;
                } else {
                    /* next cure */
                    cure = strchr(cure + 1, '\n');
                }
            }
            curl = NULL;
        } else {
            curl = strchr(curl, '\n');
        }
    }
    
    if (toret) return toret;
    
    return strdup(msg);
}

int authImport(const char *msg)
{
    /* all we can do is try, so do so */
    char *try = gpgWrap(msg, "--import", 0);
    if (try) {
        free(try);
        return 1;
    } else {
        return 0;
    }
}

char *authExport()
{
    char *cmd, *ret;
    
    CPASS return NULL;
    
    cmd = (char *) malloc((15 + strlen(GPG_name)) * sizeof(char));
    sprintf(cmd, "-a --export \"%s\"", GPG_name);
    
    ret = gpgWrap("", cmd, 1);
    free(cmd);
    
    return ret;
}
