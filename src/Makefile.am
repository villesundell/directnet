SUBDIRS = @UI_DIR@ @ENC_DIR@

CC=@PTHREAD_CC@

if UI_CXX

bin_PROGRAMS=directnetx

directnetx_SOURCES=client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c \
fake.cc

directnetx_CFLAGS=@PTHREAD_CFLAGS@ @UI_CFLAGS@ @ENC_CFLAGS@
directnetx_CXXFLAGS=@PTHREAD_CFLAGS@ @UI_CXXFLAGS@ @ENC_CXXFLAGS@
directnetx_LDFLAGS=@UI_LDFLAGS@ @ENC_LDFLAGS@
directnetx_DEPENDENCIES=@UI_DIR@/libui.a  @ENC_DIR@/libenc.a
directnetx_LDADD=@PTHREAD_CFLAGS@ @PTHREAD_LIBS@ @UI_DIR@/libui.a @ENC_DIR@/libenc.a @UI_LIBS@ @ENC_LIBS@

else

bin_PROGRAMS=directnet

directnet_SOURCES=client.c \
connection.c \
directnet.c \
hash.c \
lock.c \
server.c

directnet_CFLAGS=@PTHREAD_CFLAGS@ @UI_CFLAGS@ @ENC_CFLAGS@
directnet_LDFLAGS=@PTHREAD_CFLAGS@ @UI_LDFLAGS@ @ENC_LDFLAGS@
directnet_DEPENDENCIES=@UI_DIR@/libui.a @ENC_DIR@/libenc.a
directnet_LDADD=@PTHREAD_LIBS@ @UI_DIR@/libui.a @ENC_DIR@/libenc.a @UI_LIBS@ @ENC_LIBS@

endif
