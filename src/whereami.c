/**********************************************************************************
* Copyright (C) 2005  Gregor Richards                                             *
*                                                                                 *
* Permission is hereby granted, free of charge, to any person obtaining a copy of *
* this software and associated documentation files (the "Software"), to deal in   *
* the Software without restriction, including without limitation the rights to    *
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies   *
* of the Software, and to permit persons to whom the Software is furnished to do  *
* so, subject to the following conditions:                                        *
*                                                                                 *
* The above copyright notice and this permission notice shall be included in all  *
* copies or substantial portions of the Software.                                 *
*                                                                                 *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
* SOFTWARE.                                                                       *
**********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void dirAndFil(const char *full, char **dir, char **fil)
{
    *dir = strdup(full);
    *fil = strrchr(*dir, '/');
    if (*fil) {
        **fil = '\0';
        (*fil)++;
        *fil = strdup(*fil);
    } else {
        *fil = malloc(1);
        if (!(*fil)) {
            perror("malloc");
            exit(1);
        }
        **fil = '\0';
    }
}

char *whereAmI(const char *argvz, char **dir, char **fil)
{
    char *workd, *path, *retname;
    char *pathelem[1024];
    int i, j, osl;
    struct stat sbuf;
    
    /* 1: full path, yippee! */
    if (argvz[0] == '/') {
        dirAndFil(argvz, dir, fil);
        return strdup(argvz);
    }
    
    /* 2: relative path */
    if (strchr(argvz, '/')) {
        workd = (char *) malloc(1024 * sizeof(char));
        if (!workd) { perror("malloc"); exit(1); }
        
        if (getcwd(workd, 1024)) {
            retname = (char *) malloc((strlen(workd) + strlen(argvz) + 2) * sizeof(char));
            if (!retname) { perror("malloc"); exit(1); }
            
            sprintf(retname, "%s/%s", workd, argvz);
            free(workd);
            
            dirAndFil(retname, dir, fil);
            return retname;
        }
    }
    
    /* 3: worst case: find in PATH */
    path = getenv("PATH");
    if (path == NULL) {
        return NULL;
    }
    path = strdup(path);
    
    /* tokenize by : */
    memset(pathelem, 0, 1024 * sizeof(char *));
    pathelem[0] = path;
    i = 1;
    osl = strlen(path);
    for (j = 0; j < osl; j++) {
        for (; path[j] != '\0' && path[j] != ':'; j++);
        
        if (path[j] == ':') {
            path[j] = '\0';
            
            j++;
            pathelem[i++] = path + j;
        }
    }
    
    /* go through every pathelem */
    for (i = 0; pathelem[i]; i++) {
        retname = (char *) malloc((strlen(pathelem[i]) + strlen(argvz) + 2) * sizeof(char));
        if (!retname) { perror("malloc"); exit(1); }
        
        sprintf(retname, "%s/%s", pathelem[i], argvz);
        
        if (stat(retname, &sbuf) == -1) {
            free(retname);
            continue;
        }
        
        if (sbuf.st_mode & S_IXUSR) {
            dirAndFil(retname, dir, fil);
            return retname;
        }
        
        free(retname);
    }
    
    /* 4: can't find it */
    dir = NULL;
    fil = NULL;
    return NULL;
}
