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

#include "directnet.h"
#include "lock.h"

#ifdef HAVE_SEMAPHORE_H

#include <semaphore.h>

void dn_lockInit(DN_LOCK *lockVal)
{
    sem_init(lockVal, 1, 0);
}

void dn_lock(DN_LOCK *lockVal)
{
    sem_wait(lockVal);
}

void dn_unlock(DN_LOCK *lockVal)
{
    sem_post(lockVal);
}

#else

// No semaphores = lame (not entirely thread-safe) locking
void dn_lockInit(DN_LOCK *lockVal)
{
    *lockVal = 0;
}

void dn_lock(DN_LOCK *lockVal)
{
    while (*lockVal) sleep(0);
    *lockVal = 1;
}

void dn_unlock(DN_LOCK *lockVal)
{
    *lockVal = 0;
}

#endif
