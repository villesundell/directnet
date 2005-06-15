SUBDIRS=gmp cyfer

AM_CFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include -Lgmp -Igmp
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib -Lgmp

noinst_LIBRARIES=libenc.a
libenc_a_SOURCES=cyfer.c

install:
	$(MAKE) install-recursive SUBDIRS=""

install-exec:
	$(MAKE) install-exec-recursive SUBDIRS=""

install-data:
	$(MAKE) install-data-recursive SUBDIRS=""


