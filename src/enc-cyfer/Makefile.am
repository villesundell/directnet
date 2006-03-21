SUBDIRS=cyfer

AM_CXXFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include -Lgmp -Igmp
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib -Lgmp

noinst_LIBRARIES=libenc.a
libenc_a_SOURCES=cyfer.cc

install:
	$(MAKE) install-recursive SUBDIRS=""

install-exec:
	$(MAKE) install-exec-recursive SUBDIRS=""

install-data:
	$(MAKE) install-data-recursive SUBDIRS=""


