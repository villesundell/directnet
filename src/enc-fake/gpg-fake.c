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

#include <stdio.h>

#include "directnet.h"

char encbinloc[256];

int findEnc(char **envp)
{
    return 0;
}
    
char *encTo(const char *from, const char *to, const char *msg)
{
    return strdup(msg);
}

char *encFrom(const char *from, const char *to, const char *msg)
{
    return strdup(msg);
}

char *encCreateKey()
{
    return "";
}

char *encExportKey() {
    return "nokey";
}

char *encImportKey(const char *name, const char *key) {
    return "";
}
