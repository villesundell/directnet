#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT(directnet, [ALPHA0.3], [cdgregorr@users.sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/ui.h])
AC_CONFIG_HEADER([config.h])

AC_CONFIG_FILES(
[
Makefile
src/Makefile
src/enc-fake/Makefile
src/enc-gpg/Makefile
src/ui-curses/Makefile
src/ui-dumb/Makefile
src/ui-wx/Makefile
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
# Pthreads are absolutely necessary
ACX_PTHREAD([], AC_MSG_ERROR([ERROR: Pthreads library not found!]))
AC_SUBST(PTHREAD_CC)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

# On solaris, we need -lnsl and -lsocket
AC_CHECK_LIB([c], [socket],
        AS_MESSAGE([This is not a Solaris system.]),
        AC_CHECK_LIB([socket], [socket])
)
AC_CHECK_LIB([c], [gethostbyname],
        AS_MESSAGE([This is not a Solaris system.]),
        AC_CHECK_LIB([nsl], [gethostbyname])
)


# Figure out what UI to use
AC_ARG_ENABLE(ui,
[  --enable-ui[=dumb]
                set the UI (currently the only UI is "dumb")],
[case "$enableval" in
        dumb) ui=dumb ;;
        wx) ui=wx ;;
        wx-static) ui=wx-static ;;
        curses) ui=curses ;;
        *) AC_MSG_ERROR([Invalid UI chosen.]) ;;
esac], [ui=determine])

AS_MESSAGE([checking which UI to use...])

found_ui=no

if test "x$ui" = "xdumb" || test "x$ui" = "xdetermine"
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

if test "x$ui" = "xwx" || test "x$ui" = "xwx-static" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "xno" )
then
        AMX_OPTIONS_WXCONFIG
        AMX_PATH_WXCONFIG(2.3.4, wxWin=1)
        
        if test "x$wxWin" != x1
        then
                if test "x$ui" = "xwx"
                then
                        AC_MSG_ERROR([
The chosen UI (wx) is unconfigurable.
The wx-config script could not be found.
                        ])
                fi
        else
                if test "x$ui" != "xwx-static"
                then
                        UI_CFLAGS=$WX_CFLAGS
                        UI_CPPFLAGS=$WX_CPPFLAGS
                        UI_CXXFLAGS=$WX_CXXFLAGS
                        UI_CXX=true
                        UI_LIBS=$WX_LIBS
                        UI_LDFLAGS=
                        UI_DIR=ui-wx
                        found_ui=yes
                        ui=wx
                        AS_MESSAGE([Using wx UI])
                else
                        UI_CFLAGS=$WX_CFLAGS
                        UI_CPPFLAGS=$WX_CPPFLAGS
                        UI_CXXFLAGS=$WX_CXXFLAGS
                        UI_CXX=true
                        UI_LIBS=$WX_LIBS_STATIC
                        UI_LDFLAGS=
                        UI_DIR=ui-wx
                        found_ui=yes
                        ui=wx
                        AS_MESSAGE([Using wx UI])
                fi
        fi
fi

if test "x$ui" = "xcurses" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "no" )
then
        AC_CHECK_CURSES
        
        if test "x$has_curses" != "xtrue"
        then
                if test "x$ui" = "xcurses"
                then
                        AC_MSG_ERROR([
The chosen UI (curses) is unconfigurable.
The appropriate libraries could not be found.
                        ])
                fi
        else
                UI_CFLAGS=$CURSES_INCLUDEDIR
                UI_CPPFLAGS=
                UI_CXXFLAGS=$CURSES_INCLUDEDIR
                UI_CXX=true
                UI_LIBS=$CURSES_LIBS
                UI_LDFLAGS=
                UI_DIR=ui-curses
                found_ui=yes
                ui=curses
                AS_MESSAGE([Using curses UI])
                
                # Can we use readline?
                AC_CHECK_READLINE
                
                if test "x$has_readline" = "xtrue"
                then
                        UI_CFLAGS="$UI_CFLAGS $READLINE_INCLUDES"
                        UI_LIBS="$UI_LIBS $READLINE_LIBS"
                        AS_MESSAGE([Readline enabled])
                else
                        AS_MESSAGE([Readline disabled])
                fi
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
AM_CONDITIONAL(UI_CXX, test "x$UI_CXX" = "xtrue")


# Check for encryption to use
AC_ARG_ENABLE(enc,
[  --enable-enc[=fake]
                set the encryption (currently the only encryption is "fake")],
[case "$enableval" in
        fake) enc=fake ;;
        gpg) enc=gpg ;;
        *) AC_MSG_ERROR([Invalid encryption chosen.]) ;;
esac], [enc=determine])

AS_MESSAGE([checking which encryption to use...])

found_enc=no

if test "x$enc" = "xfake" || test "x$enc" = "xdetermine"
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

if test "x$enc" = "xgpg" || ( test "x$enc" = "xdetermine" && test "x$found_enc" = "xno" )
then
        ENC_CFLAGS=
        ENC_CPPFLAGS=
        ENC_CXXFLAGS=
        ENC_CXX=false
        ENC_LIBS=
        ENC_LDFLAGS=
        ENC_DIR=enc-gpg
        found_enc=yes
        enc=gpg
        AS_MESSAGE([Using GPG encryption])
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


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 gethostbyname gethostname inet_ntoa memset mkdir socket])
AC_OUTPUT
