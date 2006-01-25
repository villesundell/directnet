/*
 * Copyright 2005 Gregor Richards
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

#ifndef DN_FL_IPC_H
#define DN_FL_IPC_H

extern "C" {
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
    
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
}

#include "ChatWindow.h"

ChatWindow *IPC_getWindow(char mt, const char *name);
ChatWindow *getWindow(const char *name);
void IPC_putOutput(char mt, ChatWindow *w, const char *txt);
void putOutput(ChatWindow *w, const char *txt);

#define IPC_FUNC_A(t, n, p...) \
t n(char mt, p) \
{ \
    static sem_t *reqSubmitted = NULL; \
    static sem_t *reqFinished = NULL; \
    static char reqWasSub = 0; \
    static t toret;
    
#define IPC_FUNC_AV(n, p...) \
    void n(char mt, p) \
    { \
        static sem_t *reqSubmitted = NULL; \
        static sem_t *reqFinished = NULL; \
        static char reqWasSub = 0;
        
        /* STRUCT DEFINITION GOES HERE: static struct { args... } reqArgs; */
        
#define IPC_FUNC_B() \
        \
        if (!reqSubmitted) { \
            reqSubmitted = (sem_t *) malloc(sizeof(sem_t)); \
            if (!reqSubmitted) { perror("malloc"); exit(1); } \
            sem_init(reqSubmitted, 0, 1); \
        } \
        if (!reqFinished) { \
            reqFinished = (sem_t *) malloc(sizeof(sem_t)); \
            if (!reqFinished) { perror("malloc"); exit(1); } \
            sem_init(reqFinished, 0, 0); \
        } \
        \
        if (mt) { \
            if (reqWasSub) {
                
#define IPC_FUNC_BV() IPC_FUNC_B()
                
                /* ARGS PASSING AND SETTING RETURN GOES HERE:
                 * toret = realfunc(reqArgs.a, reqArgs.b, etc); */
            
#define IPC_FUNC_C(t) \
                reqWasSub = 0; \
                sem_post(reqFinished); \
                return toret; \
            } \
        } else { \
            t realret; \
            sem_wait(reqSubmitted);
        
#define IPC_FUNC_CV() \
            reqWasSub = 0; \
            sem_post(reqFinished); \
            return; \
        } \
    } else { \
        sem_wait(reqSubmitted);
        
        /* SETTING ARGS GOES HERE: reqArgs.a = blah, etc */
        
#define IPC_FUNC_D() \
        reqWasSub = 1; \
        sem_wait(reqFinished); \
        realret = toret; \
        sem_post(reqSubmitted); \
        return realret; \
    } \
}

void IPC_BPOINT();

#define IPC_FUNC_DV() \
reqWasSub = 1; \
sem_wait(reqFinished); \
sem_post(reqSubmitted); \
return; \
} \
}

#endif
