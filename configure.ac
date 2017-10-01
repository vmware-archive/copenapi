AC_INIT(copenapi, 0.1)
AC_MSG_NOTICE([copenapi configuration])

AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_PROG_AR

AC_PROG_CC
LT_INIT
AC_PROG_LIBTOOL

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE -fPIC"

AM_CPPFLAGS="$AM_CPPFLAGS -I${top_srcdir}/include" 
AM_CFLAGS="$AM_CFLAGS -Wall -Werror -Wno-unused-variable -fno-strict-aliasing -fstack-protector-strong"

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)

#jansson
PKG_CHECK_MODULES([JANSSON], [jansson], [have_libjansson=yes], [have_libjansson=no])
AM_CONDITIONAL([JANSSON],  [test "$have_libjansson" = "yes"])

#libcurl
PKG_CHECK_MODULES([LIBCURL], [libcurl], [have_libcurl=yes], [have_libcurl=no])
AM_CONDITIONAL([LIBCURL],  [test "$have_libcurl" = "yes"])

#makefiles
AC_CONFIG_FILES([Makefile
                 common/Makefile
                 lib/Makefile
                 cli/Makefile
                ])

#
AC_OUTPUT
