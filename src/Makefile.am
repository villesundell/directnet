SUBDIRS = libdirectnet

if ENABLE_UI
SUBDIRS += @UI_DIR@

bin_PROGRAMS=directnet

directnet_SOURCES=empty.cc
directnet_CFLAGS=@UI_CFLAGS@ @ENC_CFLAGS@ @AUTH_CFLAGS@
directnet_CXXFLAGS=@UI_CXXFLAGS@ @ENC_CXXFLAGS@ @AUTH_CXXFLAGS@
directnet_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@ @AUTH_LDFLAGS@
directnet_DEPENDENCIES=@UI_DIR@/libui.a libdirectnet/libdirectnet.la
directnet_LDADD=@UI_DIR@/libui.a libdirectnet/libdirectnet.la @UI_LIBS@ \
  @ENC_LIBS@ @AUTH_LIBS@
endif
