# $Id$
#
# Top-level "Makefile.am" for libupnp
#

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = ixml threadutil upnp


EXTRA_DIST	= libupnp.pc.in LICENSE


# This variable must have 'exec' in its name, in order to be installed 
# by 'install-exec' target (instead of default 'install-data')
pkgconfigexecdir 	= $(libdir)/pkgconfig
pkgconfigexec_DATA 	= libupnp.pc

$(pkgconfigexec_DATA): config.status





