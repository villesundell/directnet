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
CFLAGS=-Wall
LIBS=-lpthread

OBJS=client.o \
connection.o \
directnet.o \
gpg.o \
hash.o \
server.o \
ui.o

all: directnet

directnet: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

semiclean:
	rm -f $(OBJS) gpgtest.o

clean: semiclean
	rm -f gpgtest directnet