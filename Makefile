# $Id$
#
# Top-level Makefile for djmount
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



DEBUG=1



all: 
	cd talloc ; [ -f ?akefile ] || ./configure
	$(MAKE) -C talloc
	$(MAKE) -C libupnp/upnp DEBUG=$(DEBUG) CLIENT=1 DEVICE=0 STATIC=1
	$(MAKE) -C djmount DEBUG=$(DEBUG)
	@echo "make $@ finished on `date`"


clean:
	$(MAKE) -C djmount clean
	$(MAKE) -C libupnp/upnp clean
	-$(MAKE) -C talloc clean
	cd talloc && rm -f lib*.a


.PHONY: dist

dist:   DATE=$(shell date +'%d_%m_%Y')
dist:   BASE=$(notdir $(CURDIR))

dist:
	$(MAKE) clean
	cd .. && tar cvfz "$(BASE)_$(DATE).tgz" \
		`find $(BASE) -type f \! \( -name '*~' -o -name '*.o' \)` 
	@echo " => dist = "
	@ls -la "../$(BASE)_$(DATE).tgz"
