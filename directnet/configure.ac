#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT(directnet, [ALPHA 0.2], [cdgregorr@users.sourceforge.net])
AC_CONFIG_AUX_DIR([scripts])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/ui.h])
AC_CONFIG_HEADER([config.h])

AC_CONFIG_FILES(
[
Makefile
src/Makefile
src/ui-curses/Makefile
src/ui-dumb/Makefile
src/ui-wx/Makefile
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.

# ACX_PTHREAD check included from http://www.gnu.org/software/ac-archive/htmldoc/acx_pthread.html
AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                pthread-config)
                AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
                if test x"$acx_pthread_config" = xno; then continue; fi
                PTHREAD_CFLAGS="`pthread-config --cflags`"
                PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
                *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD

ACX_PTHREAD(, AC_MSG_ERROR([ERROR: Pthreads library not found!]))
AC_SUBST(PTHREAD_CC)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)





dnl ---------------------------------------------------------------------------
dnl Macros for wxWindows detection. Typically used in configure.in as:
dnl
dnl 	AC_ARG_ENABLE(...)
dnl 	AC_ARG_WITH(...)
dnl	...
dnl	AM_OPTIONS_WXCONFIG
dnl	...
dnl	...
dnl	AM_PATH_WXCONFIG(2.3.4, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl     	   wxWindows must be installed on your system
dnl     	   but wx-config script couldn't be found.
dnl     
dnl     	   Please check that wx-config is in path, the directory
dnl     	   where wxWindows libraries are installed (returned by
dnl     	   'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl     	   equivalent variable and wxWindows version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl     
dnl     LDFLAGS="$LDFLAGS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_WXCONFIG
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix and --wx-config 
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXCONFIG],
[
   AC_ARG_WITH(wx-prefix, [  --with-wx-prefix=PREFIX   Prefix where wxWindows is installed (optional)],
               wx_config_prefix="$withval", wx_config_prefix="")
   AC_ARG_WITH(wx-exec-prefix,[  --with-wx-exec-prefix=PREFIX Exec prefix where wxWindows is installed (optional)],
               wx_config_exec_prefix="$withval", wx_config_exec_prefix="")
   AC_ARG_WITH(wx-config,[  --with-wx-config=CONFIG   wx-config script to use (optional)],
               wx_config_name="$withval", wx_config_name="")
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_WXCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWindows, and define WX_C*FLAGS, WX_LIBS and WX_LIBS_STATIC
dnl (the latter is for static linking against wxWindows). Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wx-config script
dnl
AC_DEFUN([AM_PATH_WXCONFIG],
[
  dnl do we have wx-config name: it can be wx-config or wxd-config or ...
  if test x${WX_CONFIG_NAME+set} != xset ; then
     WX_CONFIG_NAME=wx-config
  fi
  if test "x$wx_config_name" != x ; then
     WX_CONFIG_NAME="$wx_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     WX_LOOKUP_PATH="$wx_config_exec_prefix/bin"
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     WX_LOOKUP_PATH="$WX_LOOKUP_PATH:$wx_config_prefix/bin"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wx-config)
     WX_CONFIG_PATH="$WX_CONFIG_NAME"
     AC_MSG_RESULT($WX_CONFIG_PATH)
  else
     AC_PATH_PROG(WX_CONFIG_PATH, $WX_CONFIG_NAME, no, "$WX_LOOKUP_PATH:$PATH")
  fi

  if test "$WX_CONFIG_PATH" != "no" ; then
    WX_VERSION=""
    no_wx=""

    min_wx_version=ifelse([$1], ,2.2.1,$1)
    AC_MSG_CHECKING(for wxWindows version >= $min_wx_version)

    WX_CONFIG_WITH_ARGS="$WX_CONFIG_PATH $wx_config_args"

    WX_VERSION=`$WX_CONFIG_WITH_ARGS --version`
    wx_config_major_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_config_minor_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_config_micro_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_requested_major_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_requested_minor_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_requested_micro_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_ver_ok=""
    if test $wx_config_major_version -gt $wx_requested_major_version; then
      wx_ver_ok=yes
    else
      if test $wx_config_major_version -eq $wx_requested_major_version; then
         if test $wx_config_minor_version -gt $wx_requested_minor_version; then
            wx_ver_ok=yes
         else
            if test $wx_config_minor_version -eq $wx_requested_minor_version; then
               if test $wx_config_micro_version -ge $wx_requested_micro_version; then
                  wx_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test "x$wx_ver_ok" = x ; then
      no_wx=yes
    else
      WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs`
      WX_LIBS_STATIC=`$WX_CONFIG_WITH_ARGS --static --libs`

      dnl starting with version 2.2.6 wx-config has --cppflags argument
      wx_has_cppflags=""
      if test $wx_config_major_version -gt 2; then
        wx_has_cppflags=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -gt 2; then
              wx_has_cppflags=yes
           else
              if test $wx_config_minor_version -eq 2; then
                 if test $wx_config_micro_version -ge 6; then
                    wx_has_cppflags=yes
                 fi
              fi
           fi
        fi
      fi

      if test "x$wx_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags`
         WX_CPPFLAGS=$WX_CFLAGS
         WX_CXXFLAGS=$WX_CFLAGS

         WX_CFLAGS_ONLY=$WX_CFLAGS
         WX_CXXFLAGS_ONLY=$WX_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
         WX_CPPFLAGS=`$WX_CONFIG_WITH_ARGS --cppflags`
         WX_CXXFLAGS=`$WX_CONFIG_WITH_ARGS --cxxflags`
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags`

         WX_CFLAGS_ONLY=`echo $WX_CFLAGS | sed "s@^$WX_CPPFLAGS *@@"`
         WX_CXXFLAGS_ONLY=`echo $WX_CXXFLAGS | sed "s@^$WX_CFLAGS *@@"`
      fi
    fi

    if test "x$no_wx" = x ; then
       AC_MSG_RESULT(yes (version $WX_VERSION))
       ifelse([$2], , :, [$2])
    else
       if test "x$WX_VERSION" = x; then
	  dnl no wx-config at all
	  AC_MSG_RESULT(no)
       else
	  AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
       fi

       WX_CFLAGS=""
       WX_CPPFLAGS=""
       WX_CXXFLAGS=""
       WX_LIBS=""
       WX_LIBS_STATIC=""
       ifelse([$3], , :, [$3])
    fi
  fi

  AC_SUBST(WX_CPPFLAGS)
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_CXXFLAGS)
  AC_SUBST(WX_CFLAGS_ONLY)
  AC_SUBST(WX_CXXFLAGS_ONLY)
  AC_SUBST(WX_LIBS)
  AC_SUBST(WX_LIBS_STATIC)
  AC_SUBST(WX_VERSION)
])





AC_DEFINE([HAS_CURSES], [], [Found some version of curses that we're going to use])
AC_DEFINE([NCURSES_970530], [], [Set to reflect version of ncurses
0 = version 1.*
1 = version 1.9.9g
2 = version 4.0/4.1])
AC_DEFINE([NO_COLOR_CURSES], [], [If you Curses does not have color define this one])
AC_DEFINE([SCO_FLAVOR], [], [Define if you want to turn on SCO-specific code])
AC_DEFINE([USE_NCURSES], [], [Use Ncurses?])
AC_DEFINE([USE_SUNOS_CURSES], [], [Use SunOS SysV curses?])
AC_DEFINE([USE_SYSV_CURSES], [], [Use SystemV curses?])
dnl Curses detection: Munged from Midnight Commander's configure.in
dnl
dnl What it does:
dnl =============
dnl
dnl - Determine which version of curses is installed on your system
dnl   and set the -I/-L/-l compiler entries and add a few preprocessor
dnl   symbols 
dnl - Do an AC_SUBST on the CURSES_INCLUDEDIR and CURSES_LIBS so that
dnl   @CURSES_INCLUDEDIR@ and @CURSES_LIBS@ will be available in
dnl   Makefile.in's
dnl - Modify the following configure variables (these are the only
dnl   curses.m4 variables you can access from within configure.in)
dnl   CURSES_INCLUDEDIR - contains -I's and possibly -DRENAMED_CURSES if
dnl                       an ncurses.h that's been renamed to curses.h
dnl                       is found.
dnl   CURSES_LIBS       - sets -L and -l's appropriately
dnl   CFLAGS            - if --with-sco, add -D_SVID3 
dnl   has_curses        - exports result of tests to rest of configure
dnl
dnl Usage:
dnl ======
dnl 1) Add lines indicated below to acconfig.h
dnl 2) call AC_CHECK_CURSES after AC_PROG_CC in your configure.in
dnl 3) Instead of #include <curses.h> you should use the following to
dnl    properly locate ncurses or curses header file
dnl
dnl    #if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
dnl    #include <ncurses.h>
dnl    #else
dnl    #include <curses.h>
dnl    #endif
dnl
dnl 4) Make sure to add @CURSES_INCLUDEDIR@ to your preprocessor flags
dnl 5) Make sure to add @CURSES_LIBS@ to your linker flags or LIBS
dnl
dnl Notes with automake:
dnl - call AM_CONDITIONAL(HAS_CURSES, test "$has_curses" = true) from
dnl   configure.in
dnl - your Makefile.am can look something like this
dnl   -----------------------------------------------
dnl   INCLUDES= blah blah blah $(CURSES_INCLUDEDIR) 
dnl   if HAS_CURSES
dnl   CURSES_TARGETS=name_of_curses_prog
dnl   endif
dnl   bin_PROGRAMS = other_programs $(CURSES_TARGETS)
dnl   other_programs_SOURCES = blah blah blah
dnl   name_of_curses_prog_SOURCES = blah blah blah
dnl   other_programs_LDADD = blah
dnl   name_of_curses_prog_LDADD = blah $(CURSES_LIBS)
dnl   -----------------------------------------------
dnl
dnl
dnl The following lines should be added to acconfig.h:
dnl ==================================================
dnl
dnl /*=== Curses version detection defines ===*/
dnl /* Found some version of curses that we're going to use */
dnl #undef HAS_CURSES
dnl    
dnl /* Use SunOS SysV curses? */
dnl #undef USE_SUNOS_CURSES
dnl 
dnl /* Use old BSD curses - not used right now */
dnl #undef USE_BSD_CURSES
dnl 
dnl /* Use SystemV curses? */
dnl #undef USE_SYSV_CURSES
dnl 
dnl /* Use Ncurses? */
dnl #undef USE_NCURSES
dnl 
dnl /* If you Curses does not have color define this one */
dnl #undef NO_COLOR_CURSES
dnl 
dnl /* Define if you want to turn on SCO-specific code */
dnl #undef SCO_FLAVOR
dnl 
dnl /* Set to reflect version of ncurses *
dnl  *   0 = version 1.*
dnl  *   1 = version 1.9.9g
dnl  *   2 = version 4.0/4.1 */
dnl #undef NCURSES_970530
dnl
dnl /*=== End new stuff for acconfig.h ===*/
dnl 


AC_DEFUN([AC_CHECK_CURSES],[
	search_ncurses=true
	screen_manager=""
	has_curses=false

	CFLAGS=${CFLAGS--O}

	AC_SUBST(CURSES_LIBS)
	AC_SUBST(CURSES_INCLUDEDIR)

	AC_ARG_WITH(sco,
	  [  --with-sco              Use this to turn on SCO-specific code],[
	  if test x$withval = xyes; then
		AC_DEFINE(SCO_FLAVOR)
		CFLAGS="$CFLAGS -D_SVID3"
	  fi
	])

	AC_ARG_WITH(sunos-curses,
	  [  --with-sunos-curses     Used to force SunOS 4.x curses],[
	  if test x$withval = xyes; then
		AC_USE_SUNOS_CURSES
	  fi
	])

	AC_ARG_WITH(osf1-curses,
	  [  --with-osf1-curses      Used to force OSF/1 curses],[
	  if test x$withval = xyes; then
		AC_USE_OSF1_CURSES
	  fi
	])

	AC_ARG_WITH(vcurses,
	  [  --with-vcurses[=incdir] Used to force SysV curses],
	  if test x$withval != xyes; then
		CURSES_INCLUDEDIR="-I$withval"
	  fi
	  AC_USE_SYSV_CURSES
	)

	AC_ARG_WITH(ncurses,
	  [  --with-ncurses[=dir]  Compile with ncurses/locate base dir],
	  if test x$withval = xno ; then
		search_ncurses=false
	  elif test x$withval != xyes ; then
		CURSES_LIBS="$LIBS -L$withval/lib -lncurses"
		CURSES_INCLUDEDIR="-I$withval/include"
		search_ncurses=false
		screen_manager="ncurses"
		AC_DEFINE(USE_NCURSES)
		AC_DEFINE(HAS_CURSES)
		has_curses=true
	  fi
	)

	if $search_ncurses
	then
		AC_SEARCH_NCURSES()
	fi


])


AC_DEFUN([AC_USE_SUNOS_CURSES], [
	search_ncurses=false
	screen_manager="SunOS 4.x /usr/5include curses"
	AC_MSG_RESULT(Using SunOS 4.x /usr/5include curses)
	AC_DEFINE(USE_SUNOS_CURSES)
	AC_DEFINE(HAS_CURSES)
	has_curses=true
	AC_DEFINE(NO_COLOR_CURSES)
	AC_DEFINE(USE_SYSV_CURSES)
	CURSES_INCLUDEDIR="-I/usr/5include"
	CURSES_LIBS="/usr/5lib/libcurses.a /usr/5lib/libtermcap.a"
	AC_MSG_RESULT(Please note that some screen refreshs may fail)
])

AC_DEFUN([AC_USE_OSF1_CURSES], [
       AC_MSG_RESULT(Using OSF1 curses)
       search_ncurses=false
       screen_manager="OSF1 curses"
       AC_DEFINE(HAS_CURSES)
       has_curses=true
       AC_DEFINE(NO_COLOR_CURSES)
       AC_DEFINE(USE_SYSV_CURSES)
       CURSES_LIBS="-lcurses"
])

AC_DEFUN([AC_USE_SYSV_CURSES], [
	AC_MSG_RESULT(Using SysV curses)
	AC_DEFINE(HAS_CURSES)
	has_curses=true
	AC_DEFINE(USE_SYSV_CURSES)
	search_ncurses=false
	screen_manager="SysV/curses"
	CURSES_LIBS="-lcurses"
])

dnl AC_ARG_WITH(bsd-curses,
dnl [--with-bsd-curses         Used to compile with bsd curses, not very fancy],
dnl 	search_ncurses=false
dnl	screen_manager="Ultrix/cursesX"
dnl	if test $system = ULTRIX
dnl	then
dnl	    THIS_CURSES=cursesX
dnl        else
dnl	    THIS_CURSES=curses
dnl	fi
dnl
dnl	CURSES_LIBS="-l$THIS_CURSES -ltermcap"
dnl	AC_DEFINE(HAS_CURSES)
dnl	has_curses=true
dnl	AC_DEFINE(USE_BSD_CURSES)
dnl	AC_MSG_RESULT(Please note that some screen refreshs may fail)
dnl	AC_MSG_WARN(Use of the bsdcurses extension has some)
dnl	AC_MSG_WARN(display/input problems.)
dnl	AC_MSG_WARN(Reconsider using xcurses)
dnl)

	
dnl
dnl Parameters: directory filename cureses_LIBS curses_INCLUDEDIR nicename
dnl
AC_DEFUN([AC_NCURSES], [
    if $search_ncurses
    then
        if test -f $1/$2
	then
	    AC_MSG_RESULT(Found ncurses on $1/$2)
 	    CURSES_LIBS="$3"
	    CURSES_INCLUDEDIR="$4"
	    search_ncurses=false
	    screen_manager=$5
            AC_DEFINE(HAS_CURSES)
            has_curses=true
	    AC_DEFINE(USE_NCURSES)
	fi
    fi
])

AC_DEFUN([AC_SEARCH_NCURSES], [
    AC_CHECKING("location of ncurses.h file")

    AC_NCURSES(/usr/include, ncurses.h, -lncurses,, "ncurses on /usr/include")
    AC_NCURSES(/usr/include/ncurses, ncurses.h, -lncurses, -I/usr/include/ncurses, "ncurses on /usr/include/ncurses")
    AC_NCURSES(/usr/local/include, ncurses.h, -L/usr/local/lib -lncurses, -I/usr/local/include, "ncurses on /usr/local")
    AC_NCURSES(/usr/local/include/ncurses, ncurses.h, -L/usr/local/lib -L/usr/local/lib/ncurses -lncurses, -I/usr/local/include/ncurses, "ncurses on /usr/local/include/ncurses")

    AC_NCURSES(/usr/local/include/ncurses, curses.h, -L/usr/local/lib -lncurses, -I/usr/local/include/ncurses -DRENAMED_NCURSES, "renamed ncurses on /usr/local/.../ncurses")

    AC_NCURSES(/usr/include/ncurses, curses.h, -lncurses, -I/usr/include/ncurses -DRENAMED_NCURSES, "renamed ncurses on /usr/include/ncurses")

    dnl
    dnl We couldn't find ncurses, try SysV curses
    dnl
    if $search_ncurses 
    then
        AC_EGREP_HEADER(init_color, /usr/include/curses.h,
	    AC_USE_SYSV_CURSES)
	AC_EGREP_CPP(USE_NCURSES,[
#include <curses.h>
#ifdef __NCURSES_H
#undef USE_NCURSES
USE_NCURSES
#endif
],[
	CURSES_INCLUDEDIR="$CURSES_INCLUDEDIR -DRENAMED_NCURSES"
        AC_DEFINE(HAS_CURSES)
	has_curses=true
        AC_DEFINE(USE_NCURSES)
        search_ncurses=false
        screen_manager="ncurses installed as curses"
])
    fi

    dnl
    dnl Try SunOS 4.x /usr/5{lib,include} ncurses
    dnl The flags USE_SUNOS_CURSES, USE_BSD_CURSES and BUGGY_CURSES
    dnl should be replaced by a more fine grained selection routine
    dnl
    if $search_ncurses
    then
	if test -f /usr/5include/curses.h
	then
	    AC_USE_SUNOS_CURSES
        fi
    else
        # check for ncurses version, to properly ifdef mouse-fix
	AC_MSG_CHECKING(for ncurses version)
	ncurses_version=unknown
cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#ifdef RENAMED_NCURSES
#include <curses.h>
#else
#include <ncurses.h>
#endif
#undef VERSION
VERSION:NCURSES_VERSION
EOF
        if (eval "$ac_cpp conftest.$ac_ext") 2>&AC_FD_CC |
  egrep "VERSION:" >conftest.out 2>&1; then
changequote(,)dnl
            ncurses_version=`cat conftest.out|sed -e 's/^[^"]*"//' -e 's/".*//'`
changequote([,])dnl
	fi
	rm -rf conftest*
        AC_MSG_RESULT($ncurses_version)
	case "$ncurses_version" in
changequote(,)dnl
	4.[01])
changequote([,])dnl
            AC_DEFINE(NCURSES_970530,2)
            ;;
	1.9.9g)
            AC_DEFINE(NCURSES_970530,1)
            ;;
	1*)
            AC_DEFINE(NCURSES_970530,0)
            ;;
	esac
    fi
])





# Figure out what UI to use
AC_ARG_ENABLE(ui,
[  --enable-ui[=dumb]
                set the UI (currently the only UI is "dumb")],
[case "$enableval" in
        dumb) ui=dumb ;;
        wx) ui=wx ;;
        curses) ui=curses ;;
esac], [ui=determine])

AS_MESSAGE([checking which UI to use...])

found_ui=no

if test "x$ui" = "xdumb" || test "x$ui" = "xdetermine"
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
fi

if test "x$ui" = "xwx" || ( test "x$ui" = "xdetermine" && test "x$found_ui" = "xno" )
then
        AM_OPTIONS_WXCONFIG
        AM_PATH_WXCONFIG(2.3.4, wxWin=1)
        
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
                UI_CFLAGS=$WX_CFLAGS
                UI_CPPFLAGS=$WX_CPPFLAGS
                UI_CXXFLAGS=$WX_CXXFLAGS
                UI_LIBS=$WX_LIBS
                UI_LDFLAGS=
                UI_DIR=ui-wx
                found_ui=yes
                ui=wx
                AS_MESSAGE([Using wx UI])
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
                UI_CXXFLAGS=
                UI_LIBS=$CURSES_LIBS
                UI_LDFLAGS=
                UI_DIR=ui-curses
                found_ui=yes
                ui=curses
                AS_MESSAGE([Using curses UI])
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
AM_CONDITIONAL(UI_DUMB, test "x$ui" = "xdumb")


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
