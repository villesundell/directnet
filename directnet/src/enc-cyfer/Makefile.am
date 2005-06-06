SUBDIRS=gmp cyfer

AM_CFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include -Lgmp -Igmp
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib -Lgmp

noinst_LIBRARIES=libenc.a
libenc_a_SOURCES=cyfer.c
libenc_a_DEPENDENCIES=gmp cyfer

gmp:
	cd gmp ; $(MAKE) all

cyfer: gmp
	cd cyfer ; $(MAKE) all
