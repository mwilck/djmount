#!/bin/bash
# $Id$
# 
# Test Charset conversion.
# This file is part of djmount.
# 
# (C) Copyright 2006 Rémi Turboult <r3mi@users.sourceforge.net>
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

fatal() {
    echo "" >&2
    echo "$*" >&2
    echo "**error** unexpected result" >&2
    exit 1
}


echo $0


# display charset conversion kind
res="$(./test_charset < /dev/null)" || fatal "$res"
echo "$res"

#
# Test basic conversions from various charset
# -- this does not test conversions that may fail or may not be reversible
# -- depending on environment e.g. non-ascii characters
#
for charset in '' UTF8 ASCII
do

    charset_name=${charset:-default from locale}

    #
    echo -n " - simple ascii string (charset = $charset_name) ..."
    #
    input='/tmp/some ASCII string/xxx&"#{[-|_\@)]=}$,?;:/!'
    res="$(echo $input | ./test_charset $charset 2>&1 )" || fatal "$res"
    echo " OK"

    #
    echo -n " - empty string (charset = $charset_name) ..."
    #
    input=''
    res="$(echo $input | ./test_charset $charset 2>&1 )" || fatal "$res"
    echo " OK"

    #
    echo -n " - long ascii string (charset = $charset_name) ..."
    #
    input='some ASCII string/more then 40 characters/xxxxxxxxxxxxxxxxxxx
-----------------------------------------------------------------------------
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
-----------------------------------------------------------------------------
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
'
    res="$(echo $input | ./test_charset $charset 2>&1 )" || fatal "$res"
    echo " OK"
   
done


exit 0


