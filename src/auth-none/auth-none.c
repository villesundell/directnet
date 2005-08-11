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

#include <stdlib.h>

#include "auth.h"

int authInit()
{
    return 1;
}

int authNeedPW()
{
    return 0;
}

void authSetPW(char *nm, char *pswd) {}

char *authSign(char *msg)
{
    return strdup(msg);
}

char *authVerify(char *msg, char **who, int *status)
{
    *who = NULL;
    *status = -1;
    return strdup(msg);
}

int authImport(char *msg)
{
    return 1;
}

char *authExport()
{
    return NULL;
}
