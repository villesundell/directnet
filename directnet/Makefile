# This file is part of DirectNet.
#
#    DirectNet is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    DirectNet is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with DirectNet; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

CC=gcc
CFLAGS_ALL=$(CFLAGS) -Wall
LIBS=$(LDFLAGS) -lpthread

OBJS=client.o \
connection.o \
directnet.o \
gpg-fake.o \
hash.o \
server.o \
ui.o

all: directnet

directnet: $(OBJS)
	rm -fr directnet
	$(CC) $(CFLAGS_ALL) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS_ALL) -c $< -o $@

semiclean:
	rm -fr $(OBJS) gpgtest.o

clean: semiclean
	rm -fr gpgtest directnet


# Platforms
help:
	cat compilehelp

gnulinux: directnet

aix:
	$(MAKE) directnet CC=cc CFLAGS_ALL="-qcpluscmt -DAIX"

hpux:
	$(MAKE) directnet CC=cc CFLAGS_ALL=""

hpux-gcc: directnet

macosx:
	$(MAKE) directnet CFLAGS="-DMACOSX"

solaris:
	$(MAKE) directnet CC=cc CFLAGS_ALL="-lnsl -lsocket"

solaris-gcc:
	$(MAKE) directnet LDFLAGS="-lsocket -lnsl"

tru64:
	$(MAKE) directnet CC=cc CFLAGS_ALL="-pthread"

tru64-gcc:
	$(MAKE) directnet CFLAGS="-pthread"
