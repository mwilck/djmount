# $Id$
#
# "Makefile.am" for "libupnp/upnp"
#
# Copyright (C) 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

SUBDIRS 		= doc

AM_CPPFLAGS		= -I$(srcdir)/inc \
			  -I$(top_srcdir)/threadutil/inc \
			  -I$(top_srcdir)/ixml/inc

AM_CFLAGS		= $(PTHREAD_CFLAGS)

AM_LDFLAGS		= $(PTHREAD_LIBS)

LDADD			= libupnp.la \
			  $(top_builddir)/threadutil/libthreadutil.la \
			  $(top_builddir)/ixml/libixml.la

if ENABLE_DEBUG
    AM_CPPFLAGS		+= -DDEBUG
else
    AM_CPPFLAGS		+= -DNO_DEBUG -DNDEBUG
endif

if ENABLE_CLIENT
    AM_CPPFLAGS		+= -DINCLUDE_CLIENT_APIS
endif
if ENABLE_DEVICE
    AM_CPPFLAGS		+= -DINCLUDE_DEVICE_APIS
endif
if ENABLE_WEBSERVER
    AM_CPPFLAGS 	+= -DINTERNAL_WEB_SERVER
endif


upnpincludedir          = $(includedir)/upnp
upnpinclude_HEADERS     = inc/upnp.h inc/config.h inc/upnptools.h 

lib_LTLIBRARIES		= libupnp.la

libupnp_la_CPPFLAGS	= $(AM_CPPFLAGS) -I$(srcdir)/src/inc 

libupnp_la_SOURCES	= \
			src/inc/client_table.h src/inc/global.h \
			src/inc/md5.h \
			src/inc/readwrite.h src/inc/ssdplib.h \
			src/inc/unixutil.h src/inc/urlconfig.h \
			src/inc/gmtdate.h src/inc/membuffer.h \
			src/inc/server.h src/inc/statcodes.h \
			src/inc/upnpapi.h src/inc/utilall.h \
			src/inc/gena_ctrlpt.h src/inc/http_client.h \
			src/inc/miniserver.h src/inc/service_table.h \
			src/inc/statuscodes.h src/inc/upnpclosesocket.h \
			src/inc/util.h src/inc/gena_device.h \
			src/inc/httpparser.h src/inc/netall.h \
			src/inc/soaplib.h src/inc/strintmap.h \
			src/inc/upnp_timeout.h src/inc/uuid.h \
			src/inc/gena.h src/inc/httpreadwrite.h \
			src/inc/parsetools.h src/inc/sock.h \
			src/inc/sysdep.h src/inc/uri.h \
			src/inc/webserver.h

# ssdp
libupnp_la_SOURCES	+= src/ssdp/ssdp_device.c src/ssdp/ssdp_ctrlpt.c \
			src/ssdp/ssdp_server.c

# soap
libupnp_la_SOURCES	+= src/soap/soap_device.c src/soap/soap_ctrlpt.c \
			src/soap/soap_common.c

# genlib
libupnp_la_SOURCES	+= \
			src/genlib/miniserver/miniserver.c \
			src/genlib/service_table/service_table.c \
			src/genlib/util/membuffer.c \
			src/genlib/util/strintmap.c \
			src/genlib/util/upnp_timeout.c \
			src/genlib/util/util.c \
			src/genlib/client_table/client_table.c \
			src/genlib/net/sock.c \
			src/genlib/net/http/httpparser.c \
			src/genlib/net/http/httpreadwrite.c \
			src/genlib/net/http/statcodes.c \
			src/genlib/net/http/webserver.c \
			src/genlib/net/http/parsetools.c \
			src/genlib/net/uri/uri.c

# gena
libupnp_la_SOURCES	+= src/gena/gena_device.c src/gena/gena_ctrlpt.c \
			src/gena/gena_callback2.c

# api
libupnp_la_SOURCES	+= src/api/config.c src/api/upnpapi.c
if ENABLE_TOOLS
    libupnp_la_SOURCES	+= src/api/upnptools.c
endif

# uuid
libupnp_la_SOURCES	+= src/uuid/md5.c src/uuid/sysdep.c src/uuid/uuid.c

# urlconfig
libupnp_la_SOURCES	+= src/urlconfig/urlconfig.c



check_PROGRAMS		=
if ENABLE_CLIENT
    check_PROGRAMS	+= upnp_tv_ctrlpt
    upnp_tv_ctrlpt_CPPFLAGS = $(AM_CPPFLAGS) \
			-I$(srcdir)/sample/common/ \
			-I$(srcdir)/sample/tvctrlpt
    upnp_tv_ctrlpt_SOURCES = \
			sample/common/sample_util.c \
			sample/common/sample_util.h \
			sample/tvctrlpt/upnp_tv_ctrlpt.c \
			sample/tvctrlpt/upnp_tv_ctrlpt.h \
			sample/tvctrlpt/linux/upnp_tv_ctrlpt_main.c
endif
if ENABLE_DEVICE
    check_PROGRAMS	+= upnp_tv_device 
    upnp_tv_device_CPPFLAGS = $(AM_CPPFLAGS) \
			-I$(srcdir)/sample/common/ \
			-I$(srcdir)/sample/tvdevice
    upnp_tv_device_SOURCES = \
			sample/common/sample_util.c \
			sample/common/sample_util.h \
			sample/tvdevice/upnp_tv_device.c \
			sample/tvdevice/upnp_tv_device.h \
			sample/tvdevice/linux/upnp_tv_device_main.c
endif


EXTRA_DIST 		= LICENSE





