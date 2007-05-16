SUBDIRS = @UI_DIR@ @ENC_DIR@ @AUTH_DIR@ @GAIM_PLUGIN_DIR@ 

lib_LIBRARIES=libdirectnet.a
bin_PROGRAMS=directnet

libdirectnet_a_SOURCES=binseq.cc \
chat.cc \
client.cc \
connection.cc \
dht.cc \
directnet.cc \
dnconfig.cc \
message.cc \
route.cc \
server.cc \
whereami.cc

if NO_GETTIMEOFDAY
libdirectnet_a_SOURCES+=compat/gettimeofday.c
endif

libdirectnet_a_ARFLAGS=@ENC_DIR@/*.o @AUTH_DIR@/*.o

directnet_SOURCES=empty.cc
directnet_CFLAGS=@UI_CFLAGS@ @ENC_CFLAGS@ @AUTH_CFLAGS@
directnet_CXXFLAGS=@UI_CXXFLAGS@ @ENC_CXXFLAGS@ @AUTH_CXXFLAGS@
directnet_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnet_DEPENDENCIES=@UI_DIR@/libui.a @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
directnet_LDADD=libdirectnet.a @UI_DIR@/libui.a @ENC_DIR@/libenc.a \
 @AUTH_DIR@/libauth.a @UI_LIBS@ @ENC_LIBS@ @AUTH_LIBS@


#if GAIM_PLUGIN
#
#gaimdir=$(libdir)/gaim
#
#gaim_LTLIBRARIES=libdirectnet_gaim.la
#
#libdirectnet_gaim_la_SOURCES=binseq.cc \
#chat.cc \
#client.cc \
#connection.cc \
#dht.cc \
#directnet.cc \
#dnconfig.cc \
#message.cc \
#route.cc \
#server.cc \
#whereami.cc
#
#if NO_GETTIMEOFDAY
#libdirectnet_gaim_la_SOURCES+=compat/gettimeofday.c
#endif
#
#libdirectnet_gaim_la_CFLAGS=@ENC_CFLAGS@ @AUTH_CFLAGS@ \
# @GAIM_CFLAGS@ -DGAIM_PLUGIN -DGAIM_PLUGINS
#libdirectnet_gaim_la_LDFLAGS=@ENC_LDFLAGS@ @AUTH_LDFLAGS@ \
# -module -avoid-version -Wl,--export-dynamic
#libdirectnet_gaim_la_DEPENDENCIES=@GAIM_PLUGIN_DIR@/libgaim_plugin.a \
# @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a
#libdirectnet_gaim_la_LIBADD=@GAIM_PLUGIN_DIR@/*.o \
# @ENC_DIR@/libenc.a @AUTH_DIR@/libauth.a @ENC_LIBS@ @AUTH_LIBS@ @GAIM_LIBS@
#
#endif
