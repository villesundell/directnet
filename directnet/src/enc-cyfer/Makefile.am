AM_CFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include -Lgmp -Igmp
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib -Lgmp

noinst_LIBRARIES=libenc.a
libenc_a_SOURCES=cyfer.c
libenc_a_DEPENDENCIES=gmpdir cyferdir

gmp:
	cd gmp ; $(MAKE) all

cyferdir: gmpdir
	cd cyfer ; $(MAKE) all
