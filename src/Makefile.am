SUBDIRS = @UI_DIR@ @HUB_DIR@ @ENC_DIR@ @GAIM_PLUGIN_DIR@ 

CC=@PTHREAD_CC@

if UI_CXX

if MAKE_HUB
bin_PROGRAMS=directnet directnethub
else
bin_PROGRAMS=directnet
endif
noinst_PROGRAMS=directnetcxx

directnetcxx_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c \
fake.cc

directnetcxx_CFLAGS=@PTHREAD_CFLAGS@ @UI_CFLAGS@ @ENC_CFLAGS@
directnetcxx_CXXFLAGS=@PTHREAD_CFLAGS@ @UI_CXXFLAGS@ @ENC_CXXFLAGS@
directnetcxx_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@
directnetcxx_DEPENDENCIES=@UI_DIR@/libui.a  @ENC_DIR@/libenc.a
directnetcxx_LDADD=@PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @UI_DIR@/libui.a @ENC_DIR@/libenc.a @UI_LIBS@ @ENC_LIBS@

else

if MAKE_HUB
bin_PROGRAMS=directnet directnethub
else
bin_PROGRAMS=directnet
endif
noinst_PROGRAMS=directnetc

directnetc_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c

directnetc_CFLAGS=@PTHREAD_CFLAGS@ @UI_CFLAGS@ @ENC_CFLAGS@
directnetc_LDFLAGS=@PTHREAD_CFLAGS@ @UI_LDFLAGS@ @ENC_LDFLAGS@
directnetc_DEPENDENCIES=@UI_DIR@/libui.a @ENC_DIR@/libenc.a
directnetc_LDADD=@PTHREAD_LIBS@ @UI_DIR@/libui.a @ENC_DIR@/libenc.a @UI_LIBS@ @ENC_LIBS@

endif


if MAKE_HUB

directnethub_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c

directnethub_CFLAGS=@PTHREAD_CFLAGS@ @ENC_CFLAGS@
directnethub_LDFLAGS=@PTHREAD_CFLAGS@ @ENC_LDFLAGS@
directnethub_DEPENDENCIES=@HUB_DIR@/libhub.a @ENC_DIR@/libenc.a
directnethub_LDADD=@PTHREAD_LIBS@ @HUB_DIR@/libhub.a @ENC_DIR@/libenc.a @ENC_LIBS@

endif


if GAIM_PLUGIN

gaimdir=$(libdir)/gaim

gaim_LTLIBRARIES=libdirectnet_gaim.la

libdirectnet_gaim_la_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c

libdirectnet_gaim_la_CFLAGS=@PTHREAD_CFLAGS@ @ENC_CFLAGS@ @GAIM_CFLAGS@ -DGAIM_PLUGIN -DGAIM_PLUGINS
libdirectnet_gaim_la_LDFLAGS=@PTHREAD_CFLAGS@ @ENC_LDFLAGS@ -module -avoid-version -Wl,--export-dynamic
libdirectnet_gaim_la_DEPENDENCIES=@GAIM_PLUGIN_DIR@/libgaim_plugin.a @ENC_DIR@/libenc.a
libdirectnet_gaim_la_LIBADD=@PTHREAD_LIBS@ @GAIM_PLUGIN_DIR@/libgaim_plugin.a @ENC_DIR@/libenc.a @ENC_LIBS@ @GAIM_LIBS@

endif

if UI_CXX
REALBIN=directnetcxx$(EXEEXT)
else
REALBIN=directnetc$(EXEEXT)
endif

directnet: $(REALBIN)
	cp -f $(REALBIN) $@

