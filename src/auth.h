/*
 * Copyright 2005, 2006  Gregor Richards
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

#ifndef DN_AUTH_H
#define DN_AUTH_H

#include "binseq.h"

/* Initialize authentication
 * returns: 1 on success, 0 otherwise*/
int authInit();

/* Does this authentication scheme need a password?
 * returns: 1 if it does, 0 if it doesn't */
int authNeedPW();

/* Set the password for this scheme
 * nm: the (user)name
 * pswd: the password */
void authSetPW(const char *nm, const char *pswd);

/* Sign a message
 * msg: the message
 * returns: a MALLOC'D buffer with the signed message */
BinSeq *authSign(const BinSeq &msg);

/* Verify a signature
 * msg: the message w/ signature
 * who: set to a MALLOC'D buffer with the name (and info) of the signer or NULL
 * status: set to:
 *         -1: an error occurred (no GPG, etc)
 *          0: no signature or unknown signature
 *          1: valid signature
 *          2: is a signature (not a signed message)
 * returns: a MALLOC'D buffer with the unsigned message */
BinSeq *authVerify(const BinSeq &msg, char **who, int *status);

/* Import a signature
 * msg: the signature
 * returns: 1 on success, 0 otherwise */
int authImport(const BinSeq &msg);

/* Export your signature
 * returns: a MALLOC'D buffer with the user's signature or NULL on error */
BinSeq *authExport();

/* Some text for UIs */
extern char authUsername[];
extern char authPW[];

#endif
