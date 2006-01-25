#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT(directnet, [1.0.0], [cdgregorr@users.sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/ui.h])
AC_CONFIG_HEADER([config.h])

AC_CONFIG_FILES(
[
Makefile
src/Makefile
src/auth-gpg/Makefile
src/auth-none/Makefile
src/enc-cyfer/Makefile
src/enc-fake/Makefile
src/gaim-plugin/Makefile
src/gaim2-plugin/Makefile
src/hub/Makefile
src/ui-dumb/Makefile
src/ui-fltk/Makefile
src/ui-slang/Makefile
])

if test ! -e ${srcdir}/src/enc-cyfer/gmp
then
        AC_MSG_ERROR([ERROR: You must download gmp before configuring!  Run getcyfer.sh])
fi
if test ! -e ${srcdir}/src/enc-cyfer/cyfer
then
        AC_MSG_ERROR([ERROR: You must download cyfer before configuring!  Run getcyfer.sh])
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.
# Pthreads are absolutely necessary
ACX_PTHREAD([], AC_MSG_ERROR([ERROR: Pthreads library not found!]))
AC_SUBST(PTHREAD_CC)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

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


# Figure out what UI to use
AC_ARG_ENABLE(ui,
AS_HELP_STRING([--enable-ui[[=fltk|dumb]]], [set the UI]),
[case "$enableval" in
        dumb) ui=dumb ;;
        fltk) ui=fltk ;;
        slang) ui=slang ;;
        *) AC_MSG_ERROR([Invalid UI chosen.]) ;;
esac], [ui=determine])

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
UI_CXX=false
UI_LIBS=
UI_LDFLAGS=
UI_DIR=ui-$custom_ui
AS_MESSAGE([Using custom UI: $custom_ui])
])

AC_ARG_ENABLE(custom-ui-libs,
AS_HELP_STRING([--enable-custom-ui-libs=<libs>],
[add <libs> to the linker line.  Any necessary CFLAGS are assumed to be in \$CFLAGS]),
[
UI_LIBS="$enableval"
])

AC_ARG_ENABLE(custom-ui-cxx,
AS_HELP_STRING([--enable-custom-ui-cxx],
[tells the linker that the custom UI is C++, and so it must link with a C++ compiler.  If the
custom UI is C++, you MUST use this.]),
[
UI_CXX=true
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
                UI_CXX=true
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
        UI_CXX=false
        UI_LIBS=
        UI_LDFLAGS=
        UI_DIR=ui-dumb
        found_ui=yes
        ui=dumb
        AS_MESSAGE([Using dumb UI])
fi


if test "x$ui" = "xslang" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "xno" )
then
	slang_ui=ok
	AC_CHECK_HEADER([slang/slang.h], [],
          [slang_ui=notok])
        AC_CHECK_LIB([slang], [SLtt_cls], [],
          [slang_ui=notok])

	if test "x$slang_ui" = "xok"
	then
	        UI_CFLAGS=
	        UI_CPPFLAGS=
	        UI_CXXFLAGS=
	        UI_CXX=true
	        UI_LIBS=-lslang
	        UI_LDFLAGS=
	        UI_DIR=ui-slang
	        found_ui=yes
	        ui=slang
	        AS_MESSAGE([Using slang UI])
	fi
fi


if test "x$found_ui" = "xno"
then
        AC_MSG_ERROR([Couldn't find a working UI.])
fi

AC_SUBST(UI_CFLAGS)
AC_SUBST(UI_CPPFLAGS)
AC_SUBST(UI_CXXFLAGS)
AC_SUBST(UI_LIBS)
AC_SUBST(UI_LDFLAGS)
AC_SUBST(UI_DIR)
AC_SUBST(UI_CXX)
AM_CONDITIONAL(UI_CXX, test "x$UI_CXX" = "xtrue")


# Check if we should enable the Gaim (1) plugin
AC_ARG_ENABLE(gaim-plugin,
AS_HELP_STRING([--enable-gaim-plugin],
[enable the Gaim Plugin - must have Gaim 1.2.0 or higher development libraries]),
[

GAIM_PLUGIN=yes

# Check for Gaim
PKG_CHECK_MODULES(GAIM,
[gaim >= 1.2.0 gaim < 2.0.0 gtk+-2.0],
[
        AC_DEFINE(HAVE_GAIM, 1, [Define if gaim-devel is available.])
],
[
AC_MSG_ERROR([Gaim development libraries not found.])
])

AC_SUBST(GAIM_CFLAGS)
AC_SUBST(GAIM_LIBS)

], [GAIM_PLUGIN=no], [GAIM_PLUGIN=no])

if test "x$GAIM_PLUGIN" = "xyes"
then
        GAIM_PLUGIN_DIR=gaim-plugin
else
        GAIM_PLUGIN_DIR=
fi

AC_SUBST(GAIM_PLUGIN_DIR)
AM_CONDITIONAL(GAIM_PLUGIN, test "x$GAIM_PLUGIN" = "xyes")


# Check if we should enable the Gaim2 plugin
AC_ARG_ENABLE(gaim2-plugin,
AS_HELP_STRING([--enable-gaim2-plugin],
[enable the Gaim2 Plugin - must have Gaim 2.0.0beta1 or higher development libraries]),
[

GAIM2_PLUGIN=yes

# Check for Gaim
PKG_CHECK_MODULES(GAIM,
[gaim >= 2.0.0beta1 gtk+-2.0],
[
        AC_DEFINE(HAVE_GAIM, 1, [Define if gaim-devel is available.])
],
[
AC_MSG_ERROR([Gaim development libraries not found.])
])

AC_SUBST(GAIM_CFLAGS)
AC_SUBST(GAIM_LIBS)

], [GAIM2_PLUGIN=no], [GAIM2_PLUGIN=no])

if test "x$GAIM2_PLUGIN" = "xyes"
then
        GAIM_PLUGIN_DIR=gaim2-plugin
else
        GAIM_PLUGIN_DIR=
fi

AC_SUBST(GAIM_PLUGIN_DIR)
AM_CONDITIONAL(GAIM_PLUGIN, test "x$GAIM2_PLUGIN" = "xyes")


# Check if we should enable the hub
AC_ARG_ENABLE(hub,
AS_HELP_STRING([--enable-hub=name], [enable hub mode - name is the name of the hub "user"]),
[HUB_NAME=$enableval], [HUB_NAME=no], [HUB_NAME=no])

if test "x$HUB_NAME" != "xno"
then
        HUB_DIR=hub
else
        HUB_DIR=
fi

AC_SUBST(HUB_NAME)
AC_SUBST(HUB_DIR)
AM_CONDITIONAL(MAKE_HUB, test "x$HUB_NAME" != "xno")


# Check for encryption to use
AC_ARG_ENABLE(enc,
AS_HELP_STRING([--enable-enc[[=cyfer|fake]]], [set the encryption (cyfer or fake)]),
[case "$enableval" in
        cyfer) enc=cyfer ;;
        fake) enc=fake ;;
        *) AC_MSG_ERROR([Invalid encryption chosen.]) ;;
esac], [enc=determine])

AS_MESSAGE([checking which encryption to use...])

found_enc=no

if test "x$enc" = "xcyfer" || test "x$enc" = "xdetermine"
then
        CYFCF="-L`pwd`/$srcdir/src/enc-cyfer/cyfer/lib -I`pwd`/$srcdir/src/enc-cyfer/cyfer/include"
        GMPCF="-L`pwd`/$srcdir/src/enc-cyfer/gmp -I`pwd`/$srcdir/src/enc-cyfer/gmp"
        ENC_CFLAGS="$CYFCF $GMPCF"
        ENC_CPPFLAGS=
        ENC_CXXFLAGS=
        ENC_CXX=false
        ENC_LIBS="`pwd`/$srcdir/src/enc-cyfer/cyfer/lib/.libs/libcyfer.a"
        ENC_LIBS="$ENC_LIBS `pwd`/$srcdir/src/enc-cyfer/gmp/.libs/libgmp.a"
        ENC_LDFLAGS="-L`pwd`/$srcdir/src/enc-cyfer/cyfer/lib -L`pwd`/$srcdir/src/enc-cyfer/gmp"
        ENC_DIR=enc-cyfer
        found_enc=yes
        enc=cyfer
        AS_MESSAGE([Using cyfer encryption])
        
        ORIGDIR=`pwd`
        export ORIGDIR
        cd $ORIGDIR/$srcdir/src/enc-cyfer/gmp
        CC="$CC" CFLAGS="$CFLAGS" ./configure --enable-static --disable-shared --host=$host_alias \
                --build=$build_alias || exit 1
        cd $ORIGDIR/$srcdir/src/enc-cyfer/cyfer
        CC="$CC" CFLAGS="$CFLAGS -Dbool=int -Dtrue=1 -Dfalse=0" ./configure --enable-static \
                --disable-shared --host=$host_alias --build=$build_alias || exit 1
        cd $ORIGDIR
fi

if test "x$enc" = "xfake" || (test "x$enc" = "xdetermine" && test "x$found_enc" = "xno" )
then
        ENC_CFLAGS=
        ENC_CPPFLAGS=
        ENC_CXXFLAGS=
        ENC_CXX=false
        ENC_LIBS=
        ENC_LDFLAGS=
        ENC_DIR=enc-fake
        found_enc=yes
        enc=fake
        AS_MESSAGE([Using fake encryption])
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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
#AC_FUNC_MALLOC # This test breaks on MingW
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 gethostbyname gethostname inet_ntoa memset mkdir socket])
AC_OUTPUT
