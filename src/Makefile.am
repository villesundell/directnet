SUBDIRS = @UI_DIR@ @HUB_DIR@ @ENC_DIR@ @AUTH_DIR@ @GAIM_PLUGIN_DIR@ 

bin_PROGRAMS=directnet

directnet_SOURCES=chat.cc \
client.cc \
connection.cc \
directnet.cc \
dnconfig.cc \
hash.cc \
server.cc \
whereami.cc

directnet_CFLAGS=@UI_CFLAGS@ @ENC_CFLAGS@ @AUTH_CFLAGS@
directnet_CXXFLAGS=@UI_CXXFLAGS@ @ENC_CXXFLAGS@ @AUTH_CXXFLAGS@
directnet_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnet_DEPENDENCIES=@UI_DIR@/libui.a  @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
directnet_LDADD=@UI_DIR@/libui.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a @UI_LIBS@ @ENC_LIBS@ @AUTH_LIBS@


if GAIM_PLUGIN

gaimdir=$(libdir)/gaim

gaim_LTLIBRARIES=libdirectnet_gaim.la

libdirectnet_gaim_la_SOURCES=chat.cc \
client.cc \
connection.cc \
directnet.cc \
dnconfig.cc \
hash.cc \
server.cc \
whereami.cc

libdirectnet_gaim_la_CFLAGS=@ENC_CFLAGS@ @AUTH_CFLAGS@ \
 @GAIM_CFLAGS@ -DGAIM_PLUGIN -DGAIM_PLUGINS
libdirectnet_gaim_la_LDFLAGS=@ENC_LDFLAGS@ @AUTH_LDFLAGS@ \
 -module -avoid-version -Wl,--export-dynamic
libdirectnet_gaim_la_DEPENDENCIES=@GAIM_PLUGIN_DIR@/libgaim_plugin.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
libdirectnet_gaim_la_LIBADD=@GAIM_PLUGIN_DIR@/libgaim_plugin.a \
 @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a @ENC_LIBS@ @AUTH_LIBS@ @GAIM_LIBS@

endif
