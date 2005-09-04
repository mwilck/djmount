# $Id$
#
# Makefile for djmount
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


APPS = test_charset test_upnp djmount

OBJS := log.o object.o service.o device.o device_list.o content_dir.o \
	djfs.o upnp_util.o string_util.o xml_util.o \
	charset.o

INCLUDES= 

FUSEFLAGS= -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=22


LIBUPNP=../libupnp/upnp

CPPFLAGS += -I../talloc -I../sglib -I.
LIBS += -L../talloc -ltalloc
ifeq ($(DEBUG),1)
OPT = -g -O2 
CPPFLAGS += -DDEBUG=1
DEBUG_OPTION = DEBUG=1
LIBS += -L$(LIBUPNP)/bin -lupnp 
LIBS += -L$(LIBUPNP)/bin/debug -lthreadutil_dbg -lixml 
else
DEBUG_OPTION =
OPT = -g -O2
LIBS += -L$(LIBUPNP)/bin -lupnp 
LIBS += -L$(LIBUPNP)/bin -lthreadutil -lixml 
endif


ifneq ($(wildcard /usr/include/readline/readline.h),)
CPPFLAGS += -DHAVE_READLINE=1
test_upnp: LIBS +=  -lreadline -lhistory -ltermcap
endif


ifneq ($(wildcard /usr/include/iconv.h),)
HAVE_ICONV = 1
endif
ifeq ($(HAVE_ICONV),1)
CPPFLAGS += -DHAVE_ICONV=1
else
OBJS += charset_internal.o
endif


CFLAGS += -Wall $(OPT) $(CPPFLAGS) $(FUSEFLAGS) -pthread
# -Wextra
CXXFLAGS += -Wall -Wextra \
	$(OPT) $(CPPFLAGS) $(FUSEFLAGS)
# -Wold-style-cast -Weffc++  -Woverloaded-virtual 


all: $(APPS)


djmount: fuse_main.o $(OBJS) 
	$(CC) $(CFLAGS) $+ $(LIBS) -lfuse -o $@ 

test_% : $(OBJS) test_%.o 
	$(CC) $(CFLAGS) $+ $(LIBS) -o $@ 


%.o : %.cc ./upnp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<

%.o:	%.c ./upnp
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

./upnp:
	ln -snf $(LIBUPNP)/inc ./upnp

clean:
	rm -f *.o $(APPS) ./upnp
	rm -f IUpnpInfoFile.txt IUpnpErrFile.txt



