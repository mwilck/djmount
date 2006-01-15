# -*- Autoconf -*-
# $Id$
#
# Top-level configure.ac file for libupnp
#
# Process this file with autoconf to produce a configure script.
#
# (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

AC_PREREQ(2.59)
AC_INIT(libupnp,1.2.1a-rt2, r3mi@users.sourceforge.net)

AC_CONFIG_AUX_DIR(config.aux)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_SRCDIR(upnp/inc/upnp.h)

AM_INIT_AUTOMAKE([1.9 -Wall foreign subdir-objects])

# The new "config.h" file is auto-generated (the previous one in ./upnp/inc
# has been renamed to "upnpconfig.h")
AC_CONFIG_HEADERS(config.h)

AC_REVISION([$Revision$])


#
# Check for DEBUG flag
#
RT_BOOL_ARG_ENABLE([debug], [no], [extra debugging code])


#
# Check for libupnp subsets
#

RT_BOOL_ARG_ENABLE([client], [yes], [control point code (client)])


RT_BOOL_ARG_ENABLE([device], [yes],
     [device specific code (implies --disable-webserver if disabled)])


if test "x$enable_device" = xno ; then 
	enable_webserver=no
else 
	RT_BOOL_ARG_ENABLE([webserver], [yes], [integrated mini web server])
fi
AM_CONDITIONAL(ENABLE_WEBSERVER, test x"$enable_webserver" = xyes)


RT_BOOL_ARG_ENABLE([tools], [yes], [helper APIs in upnptools.h])



#
# Checks for programs
#
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET


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

# Arrange for large-file support (can be disabled with --disable-largefile).
# Define _FILE_OFFSET_BITS and _LARGE_FILES if necessary
AC_SYS_LARGEFILE


#
# Checks for header files
#
AC_HEADER_STDC
# libupnp code doesn't use autoconf variables yet,
# so just abort if a header file is not found.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h syslog.h unistd.h],[],[AC_MSG_ERROR([required header file missing])])


#
# Checks for typedefs, structures, and compiler characteristics
#	
AC_C_CONST
AC_TYPE_SIZE_T
TYPE_SOCKLEN_T


#
# Checks for library functions
#
AC_FUNC_VPRINTF
AC_FUNC_FSEEKO


#
# Checks for POSIX Threads
#
ACX_PTHREAD([],[AC_MSG_ERROR([POSIX threads are required to build this program])])




AC_CONFIG_FILES([Makefile
		 ixml/Makefile
		 ixml/doc/Makefile
                 threadutil/Makefile
		 upnp/Makefile
		 upnp/doc/Makefile
		 libupnp.pc
                 ])

AC_OUTPUT

