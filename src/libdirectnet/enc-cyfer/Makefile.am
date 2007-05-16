SUBDIRS=cyfer

AM_CXXFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib

noinst_LIBRARIES=libenc.a
libenc_a_SOURCES=cyfer.cc

all-local:
	$(AR) x cyfer/lib/.libs/libcyfer.a

clean-local:
	-rm -f *.o

install:
	$(MAKE) install-recursive SUBDIRS=""

install-exec:
	$(MAKE) install-exec-recursive SUBDIRS=""

install-data:
	$(MAKE) install-data-recursive SUBDIRS=""


