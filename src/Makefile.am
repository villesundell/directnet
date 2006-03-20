SUBDIRS = @UI_DIR@ @HUB_DIR@ @ENC_DIR@ @AUTH_DIR@ @GAIM_PLUGIN_DIR@ 

bin_PROGRAMS=directnet

if UI_CXX

noinst_PROGRAMS=directnetcxx

directnetcxx_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
dnconfig.c \
hash.c \
server.c \
whereami.c \
fake.cc

directnetcxx_CFLAGS=@UI_CFLAGS@ @ENC_CFLAGS@ @AUTH_CFLAGS@
directnetcxx_CXXFLAGS=@UI_CXXFLAGS@ @ENC_CXXFLAGS@ @AUTH_CXXFLAGS@
directnetcxx_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnetcxx_DEPENDENCIES=@UI_DIR@/libui.a  @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
directnetcxx_LDADD=@UI_DIR@/libui.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a @UI_LIBS@ @ENC_LIBS@ @AUTH_LIBS@

else

noinst_PROGRAMS=directnetc

directnetc_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
dnconfig.c \
hash.c \
server.c \
whereami.c

directnetc_CFLAGS=@UI_CFLAGS@ @ENC_CFLAGS@ @AUTH_CFLAGS@
directnetc_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnetc_DEPENDENCIES=@UI_DIR@/libui.a @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
directnetc_LDADD=@UI_DIR@/libui.a @ENC_DIR@/libenc.a \
 @AUTH_DIR@/libauth.a @UI_LIBS@ @ENC_LIBS@ @AUTH_LIBS@

endif


if MAKE_HUB

bin_PROGRAMS+=directnethub

directnethub_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
dnconfig.c \
hash.c \
server.c \
whereami.c

directnethub_CFLAGS=@ENC_CFLAGS@ @AUTH_CFLAGS@
directnethub_LDFLAGS=@ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnethub_DEPENDENCIES=@HUB_DIR@/libhub.a @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
directnethub_LDADD=@HUB_DIR@/libhub.a @ENC_DIR@/libenc.a \
 @AUTH_DIR@/libauth.a @ENC_LIBS@ @AUTH_LIBS@

endif


if GAIM_PLUGIN

gaimdir=$(libdir)/gaim

gaim_LTLIBRARIES=libdirectnet_gaim.la

libdirectnet_gaim_la_SOURCES=chat.c \
client.c \
connection.c \
directnet.c \
dnconfig.c \
hash.c \
server.c

libdirectnet_gaim_la_CFLAGS=@ENC_CFLAGS@ @AUTH_CFLAGS@ \
 @GAIM_CFLAGS@ -DGAIM_PLUGIN -DGAIM_PLUGINS
libdirectnet_gaim_la_LDFLAGS=@ENC_LDFLAGS@ @AUTH_LDFLAGS@ \
 -module -avoid-version -Wl,--export-dynamic
libdirectnet_gaim_la_DEPENDENCIES=@GAIM_PLUGIN_DIR@/libgaim_plugin.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
libdirectnet_gaim_la_LIBADD=@GAIM_PLUGIN_DIR@/libgaim_plugin.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a @ENC_LIBS@ @AUTH_LIBS@ @GAIM_LIBS@

endif


if UI_CXX
REALBIN=directnetcxx$(EXEEXT)
else
REALBIN=directnetc$(EXEEXT)
endif

directnet$(EXEEXT): $(REALBIN)
	cp -f $(REALBIN) $@
