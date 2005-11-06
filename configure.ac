# -*- Autoconf -*-
# $Id$
#
# Top-level configure.ac file for djmount
#
# Process this file with autoconf to produce a configure script.
#
# (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

AC_PREREQ(2.59)
AC_INIT(djmount, 0.391, r3mi@users.sourceforge.net)

# share auxiliary files with sub-configure in "libupnp"
AC_CONFIG_AUX_DIR(libupnp/config.aux)
AC_CONFIG_MACRO_DIR(libupnp/m4)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_MACRO_DIR(gl/m4)

AC_CONFIG_SRCDIR(djmount/djfs.c)

AM_INIT_AUTOMAKE([1.9 -Wall std-options gnu])

AC_CONFIG_HEADERS(config.h)

AC_COPYRIGHT([Copyright (C) 2005 Rémi Turboult])
AC_REVISION([$Revision$])


#
# Check for DEBUG flag
#
RT_BOOL_ARG_ENABLE([debug], [yes], [extra debugging code])
AM_CONDITIONAL(ENABLE_DEBUG, test x"$enable_debug" = xyes)
if test x"$enable_debug" = xyes; then
	AC_DEFINE([DEBUG],1,[Define to 1 to compile debug code])
fi



#
# Checks for programs
#
AC_PROG_CC
gl_EARLY

# build static libraries by default (bundled libupnp and talloc)
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

AC_PROG_INSTALL
AC_PROG_MAKE_SET

# for pkg_config
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig



#
# Default compilation flags
#
if test x"$enable_debug" = xyes; then
	# AC_PROG_CC already sets CFLAGS to "-g -O2" by default
	:
else
	# add optimise for size
	AX_CFLAGS_GCC_OPTION([-Os])
fi
AX_CFLAGS_WARN_ALL


#
# Checks for header files
#
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h locale.h stddef.h stdint.h stdarg.h])



#
# Checks for typedefs, structures, and compiler characteristics
#
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE


#
# Checks for library functions
#
AC_CHECK_FUNCS([setlocale setxattr mktime])


#
# gnulib : configuration for imported modules
#
gl_SOURCE_BASE([gl])
gl_M4_BASE([gl/m4])
gl_MODULES([minmax])
gl_INIT


#
# FUSE : checks for library and flags
#
PKG_CHECK_MODULES(FUSE, [fuse >= 2.2])
FUSE_CFLAGS="$FUSE_CFLAGS -DFUSE_USE_VERSION=22"


#
# talloc : checks for library and includes
#
AC_MSG_CHECKING([whether to use external talloc])
AC_ARG_WITH([external-talloc], 
	AS_HELP_STRING([--with-external-talloc],
	[use external talloc library (not recommended) @<:@default=use internal bundled library@:>@]))

if test x"$with_external_talloc" = xyes; then
	AC_MSG_RESULT([yes])
	# Find the library. This macro handles the --with-talloc-prefix arg.
	RT_LIB_TALLOC([],[AC_MSG_ERROR([
** Can't find external talloc library **
Try to configure again --without-external-talloc (this is the default)
to use the internal bundled library, or try --with-talloc-prefix=DIR 
if the library is installed in non-standard location DIR/include and DIR/lib.
	])])
else
	AC_MSG_RESULT([no (bundled library)])
	# Configure bundled talloc
	sinclude(talloc/talloc/config.m4)
	AC_SUBST(TALLOC_CPPFLAGS, ['-I$(top_srcdir)/talloc/talloc'])
	AC_SUBST(TALLOC_LDFLAGS, ['-L$(top_builddir)/talloc'])
	AC_SUBST(TALLOC_LIBS, [-ltalloc])
fi
AM_CONDITIONAL(INTERNAL_TALLOC, test x"$with_external_talloc" != xyes)



#
# readline (option) : checks for readline and history compatible libraries
#
VL_LIB_READLINE



#
# iconv : checks for function in C library or libiconv 
# (AM macros from "gettext" package)
#
AM_ICONV
AM_CONDITIONAL(WANT_ICONV, test x"$am_cv_func_iconv" = xyes)
AC_CHECK_HEADERS([langinfo.h])
AM_LANGINFO_CODESET
# TBD print warning if  no setlocale or nl_langinfo(CODESET) implemented



#
# Checks for POSIX Threads
#
ACX_PTHREAD([],[AC_MSG_ERROR([POSIX threads are required to build this program])])


#
# libupnp configuration
#
# Compile only necessary subset (static client libraries), 
# plus debug code if necessary.
#
# Pb: there is no documented way to pass additional arguments to sub-configure,
# so we just use the internal variable '$ac_configure_args'.
mysave_ac_arg=$ac_configure_args
ac_configure_args="$ac_configure_args --enable-client --disable-device --disable-shared --enable-debug=$enable_debug"
AC_CONFIG_SUBDIRS(libupnp)


AC_CONFIG_FILES([Makefile
		 gl/Makefile
                 djmount/Makefile
                 ])

if test x"$with_external_talloc" != xyes; then
	AC_CONFIG_FILES([talloc/Makefile])
fi

AC_OUTPUT

