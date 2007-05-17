SUBDIRS=cyfer

AM_CXXFLAGS=@ENC_CFLAGS@ -I.. -Lcyfer/lib -Icyfer/include
AM_LDFLAGS=@ENC_LDFLAGS@ -Lcyfer/lib

lib_LTLIBRARIES=libdirectnet_enc.la
libdirectnet_enc_la_SOURCES=cyfer.cc
libdirectnet_enc_la_LDFLAGS=-static
libdirectnet_enc_la_LIBADD=-Lcyfer/lib -ldirectnet_cyfer
