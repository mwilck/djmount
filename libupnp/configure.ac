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
AC_INIT(libupnp,1.2.1a-rt1, r3mi@users.sourceforge.net)

AC_CONFIG_AUX_DIR(config.aux)
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_SRCDIR(upnp/inc/upnp.h)

AM_INIT_AUTOMAKE([-Wall std-options foreign])

# for the time being, libupnp has its own "config.h" file (see ./upnp/inc)
AC_CONFIG_HEADERS(unused_config.h)

AC_REVISION($Revision$)


#
# Check for DEBUG flag
#
RT_BOOL_ARG_ENABLE([debug], [no], [extra debugging code])
AM_CONDITIONAL(ENABLE_DEBUG, test x"$enable_debug" = xyes)


#
# Check for libupnp subsets
#

RT_BOOL_ARG_ENABLE([client], [yes], [control point code (client)])
AM_CONDITIONAL(ENABLE_CLIENT, test x"$enable_client" = xyes)


RT_BOOL_ARG_ENABLE([device], [yes],
     [device specific code (implies --disable-webserver if disabled)])
AM_CONDITIONAL(ENABLE_DEVICE, test x"$enable_device" = xyes)


test "x$enable_device" = xno && enable_webserver=no
RT_BOOL_ARG_ENABLE([webserver], [yes], [integrated mini web server])
test "x$enable_device" = xno && enable_webserver=no
AM_CONDITIONAL(ENABLE_WEBSERVER, test x"$enable_webserver" = xyes)


RT_BOOL_ARG_ENABLE([tools], [yes], [helper APIs in upnptools.h])
AM_CONDITIONAL(ENABLE_TOOLS, test x"$enable_tools" = xyes)



#
# Checks for programs
#
AC_PROG_CC
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


#
# Checks for library functions
#
AC_FUNC_VPRINTF


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

