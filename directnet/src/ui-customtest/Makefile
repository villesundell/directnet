CC=gcc
CFLAGS=
AR=ar

OBJS=ui.o

all: $(OBJS)
	$(AR) rcu libui.a $(OBJS)

.SUFFIXES: .c .o

.c.o:
	$(CC) -I.. $(CFLAGS) -c $< -o $@


clean:
	rm -f libui.a $(OBJS)

distclean: clean
