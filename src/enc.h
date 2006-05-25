/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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

#ifndef DN_GPG_H
#define DN_GPG_H

#include "binseq.h"

/* Find any necessary binaries
 *   - should be called encInit
 * envp: envp from main
 * returns: 0 on success, -1 otherwise */
int findEnc(char **envp);

/* Encrypt a message to a user
 * from: the local user
 * to: the remote user
 * msg: the message
 * returns: a MALLOC'D buffer with the encrypted message or NULL on failure */
BinSeq encTo(const BinSeq &from, const BinSeq &to, const BinSeq &msg);

/* Decrypt a message from a user
 * from: the remote user
 * to: the local user
 * msg: the message
 * returns: a MALLOC'D buffer with the decrypted message or NULL on failure */
BinSeq encFrom(const BinSeq &from, const BinSeq &to, const BinSeq &msg);

/* Create the encryption key
 * returns: the created key or NULL on failure */
BinSeq encCreateKey();

/* Export the encryption key
 * returns: the encryption key or NULL on failure */
BinSeq encExportKey();

/* Hash the encryption key
 * returns: a hash of the key */
BinSeq encHashKey(const BinSeq &key);

/* Import an encryption key
 * name: the name of the owner of the key
 * key: the key
 * returns: some message describing the import, usually "" */
BinSeq encImportKey(const BinSeq &name, const BinSeq &key);

#endif // DN_GPG_H
