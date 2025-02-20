#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT(directnet, [2.0a9], [cdgregorr@users.sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/libdirectnet/directnet.cc])
AC_CONFIG_HEADER([config.h])

AC_CONFIG_FILES(
[
Makefile
docs/Makefile
src/Makefile
src/libdirectnet/Makefile
src/libdirectnet/auth-gpg/Makefile
src/libdirectnet/enc-cyfer/Makefile
src/ui-dumb/Makefile
src/ui-fltk/Makefile
src/ui-java/Makefile
])

if test ! -e ${srcdir}/src/libdirectnet/enc-cyfer/cyfer
then
        AC_MSG_ERROR([ERROR: You must download cyfer before configuring!  Run getcyfer.sh])
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for libraries.
# On solaris, we need -lnsl and -lsocket
AC_CHECK_LIB([c], [socket],
        AS_MESSAGE([This is not a Solaris or Win32 system.]),
	AC_CHECK_LIB([socket], [socket])
)
AC_CHECK_LIB([c], [gethostbyname],
        AS_MESSAGE([This is not a Solaris or Win32 system.]),
	AC_CHECK_LIB([nsl], [gethostbyname])
)

# If we have libwsock32, we probably want it
AC_CHECK_LIB([wsock32], [main])

AC_CHECK_FUNC(gettimeofday,[
 AC_DEFINE(HAVE_GETTIMEOFDAY, 1, [Defined if gettimeofday() is available])
 HAVE_GETTIMEOFDAY=true
], [
 AC_CHECK_FUNCS([ftime])
 HAVE_GETTIMEOFDAY=false
])
AM_CONDITIONAL(NO_GETTIMEOFDAY, test "x$HAVE_GETTIMEOFDAY" = "xfalse")


# Figure out what UI to use
AC_ARG_ENABLE(ui,
AS_HELP_STRING([--enable-ui[[=fltk|dumb]]], [set the UI]),
[ENABLE_UI=yes
 case "$enableval" in
        dumb) ui=dumb ;;
        fltk) ui=fltk ;;
        java) ui=java ;;
        no) ENABLE_UI=no; ui=none ;;
        *) AC_MSG_ERROR([Invalid UI chosen.]) ;;
esac], [ENABLE_UI=yes; ui=determine])

AS_MESSAGE([checking which UI to use...])

found_ui=no


AC_ARG_ENABLE(custom-ui,
AS_HELP_STRING([--enable-custom-ui=<ui>],
[use a custom UI.  See the ui-customtest directory and --enable-custom-ui=customtest for an example]),
[
custom_ui="$enableval"
ui=custom
found_ui=yes
UI_CFLAGS=
UI_CXXFLAGS=
UI_LIBS=
UI_LDFLAGS=
UI_DIR=ui-$custom_ui
AS_MESSAGE([Using custom UI: $custom_ui])

# Look for forced custom flags
ORIGDIR=`pwd`
cd $ORIGDIR/$srcdir/src/$UI_DIR || exit 1
UI_LIBS="`make listlibs CC="$CC" CXX="$CXX" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"`"
cd $ORIGDIR
])

AC_ARG_ENABLE(custom-ui-libs,
AS_HELP_STRING([--enable-custom-ui-libs=<libs>],
[add <libs> to the linker line.  Any necessary CFLAGS are assumed to be in \$CFLAGS]),
[
UI_LIBS="$enableval"
])


if test "x$ui" = "xfltk" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "xno" )
then
        AMX_PATH_FLTK([1.1], [fltk=1])

        if test "x$fltk" != x1
        then
                if test "x$ui" = "xfltk"
                then
                        AC_MSG_ERROR([
The chosen UI (fltk) is unconfigurable.
The fltk-config script could not be found.
                        ])
                fi
        else
                UI_CFLAGS=$FLTK_CFLAGS
                UI_CXXFLAGS=$FLTK_CFLAGS
                UI_LIBS=$FLTK_LIBS
                UI_LDFLAGS=
                UI_DIR=ui-fltk
                found_ui=yes
                ui=fltk
                AS_MESSAGE([Using fltk UI])
        fi
fi


if test "x$ui" = "xdumb" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "xno" )
then
        UI_CFLAGS=
        UI_CPPFLAGS=
        UI_CXXFLAGS=
        UI_LIBS=
        UI_LDFLAGS=
        UI_DIR=ui-dumb
        found_ui=yes
        ui=dumb
        AS_MESSAGE([Using dumb UI])
        AC_CHECK_LIB([event], [event_loop], [
                      LIBS="-levent $LIBS"
                      ], [AC_MSG_FAILURE([libevent not found])])
        AC_CHECK_HEADERS([event.h], [], [AC_MSG_FAILURE([event.h not found])])
fi


if test "x$ui" = "xjava"
then
        UI_CFLAGS=
        UI_CPPFLAGS=
        UI_CXXFLAGS=
        UI_LIBS=
        UI_LDFLAGS=
        UI_DIR=ui-java
        found_ui=yes
        ui=java
        AS_MESSAGE([Using Java UI])
        AC_CHECK_LIB([event], [event_loop], [
                      LIBS="-levent $LIBS"
                      ], [AC_MSG_FAILURE([libevent not found])])
        AC_CHECK_HEADERS([event.h], [], [AC_MSG_FAILURE([event.h not found])])
fi


if test "x$ui" = "xnone"
then
        AS_MESSAGE([Using no UI])
fi


if test "x$ENABLE_UI" = "xyes" && "x$found_ui" = "xno"
then
        AC_MSG_ERROR([Couldn't find a working UI. If you do not wish to build a UI, use --disable-ui.])
fi

AM_CONDITIONAL(ENABLE_UI, test "x$ENABLE_UI" = "xyes")
AC_SUBST(UI_CFLAGS)
AC_SUBST(UI_CPPFLAGS)
AC_SUBST(UI_CXXFLAGS)
AC_SUBST(UI_LIBS)
AC_SUBST(UI_LDFLAGS)
AC_SUBST(UI_DIR)


# Check for encryption to use
AC_ARG_ENABLE(enc,
AS_HELP_STRING([--enable-enc[[=cyfer]]], [set the encryption (only cyfer works)]),
[case "$enableval" in
        cyfer) enc=cyfer ;;
        *) AC_MSG_ERROR([Invalid encryption chosen.]) ;;
esac], [enc=determine])

AS_MESSAGE([checking which encryption to use...])

found_enc=no

if test "x$enc" = "xcyfer" || test "x$enc" = "xdetermine"
then
        AC_CHECK_LIB([gmp], [__gmpz_export],[
        ENC_CFLAGS="-L`pwd`/$srcdir/src/libdirectnet/enc-cyfer/cyfer/lib -I`pwd`/$srcdir/src/libdirectnet/enc-cyfer/cyfer/include"
        ENC_CPPFLAGS=
        ENC_CXXFLAGS=
        ENC_CXX=false
        ENC_LIBS="-lgmp"
        ENC_LDFLAGS=
        ENC_DIR=enc-cyfer
        found_enc=yes
        enc=cyfer
        AS_MESSAGE([Using cyfer encryption])
        
        ORIGDIR=`pwd`
        export ORIGDIR
        cd $ORIGDIR/$srcdir/src/libdirectnet/enc-cyfer/cyfer
        CC="$CC" CFLAGS="$CFLAGS -Dbool=int -Dtrue=1 -Dfalse=0" ./configure \
                --prefix=${prefix} --host=$host_alias --build=$build_alias || exit 1
        cd $ORIGDIR
        ], [AC_MSG_ERROR([You need GMP to support Cyfer encryption.])])
fi

if test "x$found_enc" = "xno"
then
        AC_MSG_ERROR([Couldn't find a working encryption library.])
fi

AC_SUBST(ENC_CFLAGS)
AC_SUBST(ENC_CPPFLAGS)
AC_SUBST(ENC_CXXFLAGS)
AC_SUBST(ENC_LIBS)
AC_SUBST(ENC_LDFLAGS)
AC_SUBST(ENC_DIR)


# Check which authentication to use
AC_ARG_ENABLE(auth,
AS_HELP_STRING([--enable-auth[[=gpg|none]]], [set the encryption (gpg or none)]),
[case "$enableval" in
        gpg) auth=gpg ;;
        none) auth=none ;;
        *) AC_MSG_ERROR([Invalid authentication chosen.]) ;;
esac], [auth=determine])

AS_MESSAGE([checking which authentication to use...])

found_auth=no

if test "x$auth" = "xgpg" || test "x$auth" = "xdetermine"
then
        AUTH_CFLAGS=
        AUTH_CPPFLAGS=
        AUTH_CXXFLAGS=
        AUTH_CXX=false
        AUTH_LIBS=
        AUTH_LDFLAGS=
        AUTH_DIR=auth-gpg
        found_auth=yes
        auth=gpg
        AS_MESSAGE([Using GPG authentication.])
fi

if test "x$auth" = "xnone" || (test "x$auth" = "xdetermine" && test "x$found_auth" = "xno" )
then
        AUTH_CFLAGS=
        AUTH_CPPFLAGS=
        AUTH_CXXFLAGS=
        AUTH_CXX=false
        AUTH_LIBS=
        AUTH_LDFLAGS=
        AUTH_DIR=auth-none
        found_auth=yes
        auth=none
        AS_MESSAGE([Using no authentication.])
fi

if test "x$found_auth" = "xno"
then
        AC_MSG_ERROR([Couldn't find a working authentication library.])
fi

AC_SUBST(AUTH_CFLAGS)
AC_SUBST(AUTH_CPPFLAGS)
AC_SUBST(AUTH_CXXFLAGS)
AC_SUBST(AUTH_LIBS)
AC_SUBST(AUTH_LDFLAGS)
AC_SUBST(AUTH_DIR)


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 getcwd gethostbyname gethostname gettimeofday inet_ntoa \
memmove memset mkdir select socket strchr strdup strerror strrchr])
AC_OUTPUT

echo 'Configuration:'
echo '              UI: '$ui
echo '      Encryption: '$enc
echo '  Authentication: '$auth
