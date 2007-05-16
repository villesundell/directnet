/*
 * Copyright 2006  Gregor Richards
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

#ifndef DN_MESSAGE_H
#define DN_MESSAGE_H

#include <vector>

#include "directnet/binseq.h"

namespace DirectNet {
    /* A convenient class for representing DirectNet messages */
    class Message {
        public:
        Message(char stype, const char *scmd, char vera, char verb);
        Message(const BinSeq &buf);
    
        BinSeq toBinSeq();
        void debug(std::string start);
    
        char type;
        BinSeq cmd;
        char ver[2];
        std::vector<BinSeq> params;
    };

    /* Mildly connected things: convert a character array to an int and back */
    int charrayToInt(const char *c);
    void intToCharray(int i, char *c);
}

#endif
