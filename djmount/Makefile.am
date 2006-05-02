# $Id$
#
# Makefile.am for djmount
#
# (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

if INTERNAL_LIBUPNP

    # Local install of UPnP headers, to find <upnp/*.h> even when
    # they are not installed
    UPNP_TMPINC		= @abs_builddir@/_upnp_temp_install

    LIBUPNP_CFLAGS	+= -I$(UPNP_TMPINC)

endif

AM_CFLAGS		= -I$(top_srcdir)/gl \
			  $(TALLOC_CFLAGS) $(LIBUPNP_CFLAGS) $(FUSE_CFLAGS) 

LDADD			= -L$(top_builddir)/gl -lgnu \
			  $(TALLOC_LIBS) $(LIBUPNP_LIBS) $(FUSE_LIBS) 
if ENABLE_CHARSET
    LDADD		+= $(LIBICONV)
endif


bin_PROGRAMS 		= djmount
noinst_PROGRAMS		= test_upnp

check_PROGRAMS 		= test_charset test_ptr_list test_string
# auto run some tests
TESTS			= test_ptr_list test_string


COMMON_SRCS 		= log.c object.c service.c \
			  device.c device_list.c didl_object.c \
			  media_file.c file_buffer.c \
			  content_dir.c djfs.c upnp_util.c \
			  string_util.c xml_util.c ptr_list.c \
			  cache.c
if ENABLE_CHARSET
    COMMON_SRCS 	+= charset.c
if !WANT_ICONV
    COMMON_SRCS 	+= charset_internal.c
endif
endif

noinst_HEADERS		= \
			log.h object.h object_p.h service.h service_p.h \
		  	device.h device_list.h didl_object.h \
			media_file.h file_buffer.h \
		  	content_dir.h content_dir_p.h djfs.h upnp_util.h \
		  	string_util.h xml_util.h ptr_list.h talloc_util.h \
			cache.h \
		  	charset.h charset_internal.h

djmount_SOURCES		= $(COMMON_SRCS) fuse_main.c

test_upnp_SOURCES	= $(COMMON_SRCS) test_upnp.c
test_upnp_LDADD		= $(LDADD) $(READLINE_LIBS)

test_charset_SOURCES	= $(COMMON_SRCS) test_charset.c

test_ptr_list_SOURCES	= $(COMMON_SRCS) test_ptr_list.c

test_string_SOURCES	= $(COMMON_SRCS) test_string.c


CLEANFILES		= IUpnpErrFile.txt IUpnpInfoFile.txt


if INTERNAL_LIBUPNP

    #
    # Local install of UPnP headers, to find <upnp/*.h> even when
    # they are not installed
    #
    UPNP_TMPHEADERS	= $(UPNP_TMPINC)/upnp/upnp.h \
			  $(UPNP_TMPINC)/upnp/ithread.h \
			  $(UPNP_TMPINC)/upnp/ixml.h

    BUILT_SOURCES	= $(UPNP_TMPHEADERS)

$(UPNP_TMPHEADERS) :
	-rm -fr $(UPNP_TMPINC)
	$(MAKE) -C $(top_builddir)/libupnp install-data \
		includedir=$(UPNP_TMPINC)


clean-local:
	-rm -fr $(UPNP_TMPINC)

endif



