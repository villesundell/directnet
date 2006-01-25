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

extern "C" {
#include <pthread.h>
}

#include "ChatWindow.h"
#include "ipc.h"

/* ChatWindow *IPC_getWindow(char mt, const char *name) { */
IPC_FUNC_A(ChatWindow *, IPC_getWindow, const char *name)
static struct { const char *name; } reqArgs;
IPC_FUNC_B();
toret = getWindow(reqArgs.name);
IPC_FUNC_C(ChatWindow *);
reqArgs.name = name;
IPC_FUNC_D();
/* } */

/* void IPC_putOutput(char mt, ChatWindow *w, const char *txt) { */
IPC_FUNC_AV(IPC_putOutput, ChatWindow *w, const char *txt)
static struct { ChatWindow *w; const char *txt; } reqArgs;
IPC_FUNC_BV()
putOutput(reqArgs.w, reqArgs.txt);
IPC_FUNC_CV();
reqArgs.w = w;
reqArgs.txt = txt;
IPC_FUNC_DV();
/* } */

void IPC_BPOINT() { printf("BREAKPOINT\n"); }
