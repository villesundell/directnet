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

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "directnet.h"

char gpghomedir[256], gpgbinloc[256];

int findGPG(char **envp)
{
    char path[1024];
    char *paths[50];
    int onpath, i, ostrlen;
    struct stat filestat;

    path[0] = '\0';
    
    for (i = 0; envp[i] != NULL; i++) {
        if (!strncmp(envp[i], "PATH=", 5)) {
            strncpy(path, envp[i]+5, 1024);
        }
    }

    paths[0] = path;
    onpath = 1;
    
    ostrlen = strlen(path);
    for (i = 0; i < ostrlen; i++) {
        if (path[i] == ':') {
            path[i] = '\0';
            paths[onpath] = path+i+1;
            onpath++;
        }
    }
    
    for (i = 0; i < onpath; i++) {
        sprintf(gpgbinloc, "%s/gpg", paths[i]);
        if (stat(gpgbinloc, &filestat) == 0) {
            if (filestat.st_mode % 2) {
                return 0;
            }
        }
    }
    
    return -1;
}    

char *gpgWrap(char *cmd, char *msg, int hasout)
{
    static char gpgresult[65536];
    char filetemp_c[1024];
    int filedes[2];
    int proc, filetemp_fd;
    int i, newari, origstrlen;
    char *newargv[255];
    int oldstdin, oldstdout, oldstderr;
    
    if (pipe(filedes) == -1) {
        perror("pipe");
        return "";
    }
    
    sprintf(filetemp_c, "/tmp/gpg-%d", getpid());
    filetemp_fd = open(filetemp_c, O_WRONLY|O_CREAT, 0600);

    if (filetemp_fd == -1) {
        perror("open(w)");
        exit(-1);
    }
    
    write(filetemp_fd, msg, strlen(msg));
    if (close(filetemp_fd) == -1) {
        perror("close");
        exit(-1);
    }
    
    oldstdin = dup(0);
    oldstdout = dup(1);
    oldstderr = dup(2);
    
    filetemp_fd = open(filetemp_c, O_RDONLY);
    if (filetemp_fd == -1) {
        perror("open(r)");
        exit(-1);
    }
        
    newargv[0] = cmd;
    newari = 1;
    origstrlen = strlen(cmd);
    for (i = 0; i <= origstrlen; i++) {
        if (cmd[i] == ' ') {
            cmd[i] = '\0';
            newargv[newari] = cmd+i+1;
            newari++;
        } else if (cmd[i] == '\0') {
            newargv[newari] = NULL;
        }
    }
        
    if (dup2(filetemp_fd, 0) == -1) {
        perror("dup2(0)");
        exit(-1);
    }
        
    if (hasout) {
        if (dup2(filedes[1], 1) == -1) {
            perror("dup2(1)");
            exit(-1);
        }
        close(2);
    }
    
    proc = fork();
    
    if (!proc) {
        execvp(cmd, newargv);
        
        printf("BIG ErROR!  Couldn't call GPG!\n");
        exit(-1);
    }
    
    dup2(oldstdin, 0);
    dup2(oldstdout, 1);
    dup2(oldstderr, 2);
    
    if (hasout) {
        gpgresult[read(filedes[0], gpgresult, 65535)] = '\0';
    } else {
        gpgresult[0] = '\0';
    }
    
    waitpid(proc, NULL, 0);
    
    unlink(filetemp_c);
    
    close(filetemp_fd);
    close(filedes[1]);
    close(filedes[0]);
    
    return gpgresult;
}
    
char *gpgTo(char *from, char *to, char *msg)
{
    char gpgcmd[1024];
    sprintf(gpgcmd, "%s --homedir %s -aes -u %s -r %s --trust-model always --no-tty", gpgbinloc, gpghomedir, from, to);
    return gpgWrap(gpgcmd, msg, 1);
}

char *gpgFrom(char *to, char *msg)
{
    char gpgcmd[1024];
    sprintf(gpgcmd, "%s --homedir %s -d -u %s --trust-model always --no-tty", gpgbinloc, gpghomedir, to);
    return gpgWrap(gpgcmd, msg, 1);
}

char *gpgCreateKey()
{
    char msg[2048], gpgcmd[1024];
    
    // Any errors here can be ignored
    sprintf(gpgcmd, "%s/pubring.gpg", gpghomedir);
    unlink(gpgcmd);
    sprintf(gpgcmd, "%s/secring.gpg", gpghomedir);
    unlink(gpgcmd);
    sprintf(gpgcmd, "%s/trustdb.gpg", gpghomedir);
    unlink(gpgcmd);
    
    mkdir(gpghomedir, 0700);
    
    sprintf(msg, "Key-Type: 1\n\
Key-Length: 1024\n\
Name-Real: %s\n\
Name-Email: %s@direct.net\n\
Expire-Date: 0\n\
%%commit\n\
%%echo Your public key has been created.\n", dn_name, dn_name);

    sprintf(gpgcmd, "%s --homedir %s --gen-key --batch", gpgbinloc, gpghomedir);
    return gpgWrap(gpgcmd, msg, 0);
}

char *gpgExportKey() {
    char gpgcmd[1024];
    sprintf(gpgcmd, "%s --homedir %s -a --export %s", gpgbinloc, gpghomedir, dn_name);
    return gpgWrap(gpgcmd, "", 1);
}

char *gpgImportKey(char *key) {
    char gpgcmd[1024];
    sprintf(gpgcmd, "%s --homedir %s -a --import", gpgbinloc, gpghomedir);
    return gpgWrap(gpgcmd, key, 0);
}
