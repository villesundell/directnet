/*
 * Copyright 2004, 2005 Gregor Richards
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

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "connection.h"
#include "directnet.h"
#include "gpg.h"
#include "ui.h"
}

extern "C" int uiInit(int argc, char ** argv, char **envp)
{
}

extern "C" int handleUInput(char *inp)
{
}

extern "C" void uiDispMsg(char *from, char *msg)
{
}

extern "C" void uiDispChatMsg(char *chat, char *from, char *msg)
{
}

extern "C" void uiEstConn(char *from)
{
}

extern "C" void uiEstRoute(char *from)
{
}

extern "C" void uiLoseConn(char *from)
{
}

extern "C" void uiLoseRoute(char *from)
{
}

extern "C" void uiNoRoute(char *to)
{
}
