#!/bin/bash
# $Id$
# 
# Test VFS object.
# This file is part of djmount.
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



mntpoint=$( mktemp -d ${TMPDIR:-/tmp}/test_vfs.XXXXXX ) || exit 1

# Delete temporaries on exit, hangup, interrupt, quit, termination
trap "cd / ; fusermount -u $mntpoint 2> /dev/null ; rmdir $mntpoint" EXIT HUP INT QUIT TERM

./test_vfs $mntpoint || exit 1


cd $mntpoint || exit 1

diff -u - <(find . -print) <<-EOF || exit 1 
	.
	./atest
	./atest/test
	./atest/test/a1
	./atest/test/a2
	./atest/test/a2/b1
	./atest/test/a2/b1/f1
	./atest/test/a2/b2
	./atest/test/a2/b2/c1
	./atest/test/a3
	./atest/test/a3/b3
	./atest/test/a3/b4
	./atest/test/a3/b4/toto
	./atest/test/a3/b5
	./atest/test/a3/b5/toto
	./atest/test/a3/b6
	./atest/test/a3/b6/toto
	./atest/test/a3/b7
	./atest/test/a3/b7/toto
	./atest/test/a3/b8
	./atest/test/a3/b8/toto
	./atest/test/a3/b9
	./atest/test/a3/b9/toto
	./atest/link_to_test
	./test
	./test/a1
	./test/a2
	./test/a2/b1
	./test/a2/b1/f1
	./test/a2/b2
	./test/a2/b2/c1
	./test/a3
	./test/a3/b3
	./test/a3/b4
	./test/a3/b4/toto
	./test/a3/b5
	./test/a3/b5/toto
	./test/a3/b6
	./test/a3/b6/toto
	./test/a3/b7
	./test/a3/b7/toto
	./test/a3/b8
	./test/a3/b8/toto
	./test/a3/b9
	./test/a3/b9/toto
	./void_file
	./broken_link
	./zetest
	./zetest/test
	./zetest/test/a1
	./zetest/test/a2
	./zetest/test/a2/b1
	./zetest/test/a2/b1/f1
	./zetest/test/a2/b2
	./zetest/test/a2/b2/c1
	./zetest/test/a3
	./zetest/test/a3/b3
	./zetest/test/a3/b4
	./zetest/test/a3/b4/toto
	./zetest/test/a3/b5
	./zetest/test/a3/b5/toto
	./zetest/test/a3/b6
	./zetest/test/a3/b6/toto
	./zetest/test/a3/b7
	./zetest/test/a3/b7/toto
	./zetest/test/a3/b8
	./zetest/test/a3/b8/toto
	./zetest/test/a3/b9
	./zetest/test/a3/b9/toto
	./zetest/link_to_test
	./.debug
	./.debug/talloc_total
	./.debug/talloc_report
	./.debug/talloc_report_full
EOF


echo -n "essais" | diff - ./atest/test/a2/b1/f1 || exit 1

# Make sure "ls -l" display in standard C locale e.g. month names
export LANG=C
export LC_ALL=C


diff -u - <(/bin/ls -lR) <<-EOF || exit 1 
.:
total 2
dr-xr-xr-x  3 root root 512 Jan  1  2000 atest
lr--r--r--  1 root root  11 Jan  1  2000 broken_link -> broken/link
dr-xr-xr-x  5 root root 512 Jan  1  2000 test
-r--r--r--  1 root root   0 Jan  1  2000 void_file
dr-xr-xr-x  3 root root 512 Jan  1  2000 zetest

./atest:
total 1
lr--r--r--  1 root root   4 Jan  1  2000 link_to_test -> test
dr-xr-xr-x  5 root root 512 Jan  1  2000 test

./atest/test:
total 2
dr-xr-xr-x  2 root root 512 Jan  1  2000 a1
dr-xr-xr-x  4 root root 512 Jan  1  2000 a2
dr-xr-xr-x  9 root root 512 Jan  1  2000 a3

./atest/test/a1:
total 0

./atest/test/a2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 b1
dr-xr-xr-x  3 root root 512 Jan  1  2000 b2

./atest/test/a2/b1:
total 1
-r--r--r--  1 root root 6 Jun 25  2004 f1

./atest/test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 c1

./atest/test/a2/b2/c1:
total 0

./atest/test/a3:
total 4
dr-xr-xr-x  2 root root 512 Jan  1  2000 b3
dr-xr-xr-x  3 root root 512 Jun 25  2004 b4
dr-xr-xr-x  3 root root 512 Jun 25  2004 b5
dr-xr-xr-x  3 root root 512 Jun 25  2004 b6
dr-xr-xr-x  3 root root 512 Jun 25  2004 b7
dr-xr-xr-x  3 root root 512 Jun 25  2004 b8
dr-xr-xr-x  3 root root 512 Jun 25  2004 b9

./atest/test/a3/b3:
total 0

./atest/test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b4/toto:
total 0

./atest/test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b5/toto:
total 0

./atest/test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b6/toto:
total 0

./atest/test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b7/toto:
total 0

./atest/test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b8/toto:
total 0

./atest/test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./atest/test/a3/b9/toto:
total 0

./test:
total 2
dr-xr-xr-x  2 root root 512 Jan  1  2000 a1
dr-xr-xr-x  4 root root 512 Jan  1  2000 a2
dr-xr-xr-x  9 root root 512 Jan  1  2000 a3

./test/a1:
total 0

./test/a2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 b1
dr-xr-xr-x  3 root root 512 Jan  1  2000 b2

./test/a2/b1:
total 1
-r--r--r--  1 root root 6 Jun 25  2004 f1

./test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 c1

./test/a2/b2/c1:
total 0

./test/a3:
total 4
dr-xr-xr-x  2 root root 512 Jan  1  2000 b3
dr-xr-xr-x  3 root root 512 Jun 25  2004 b4
dr-xr-xr-x  3 root root 512 Jun 25  2004 b5
dr-xr-xr-x  3 root root 512 Jun 25  2004 b6
dr-xr-xr-x  3 root root 512 Jun 25  2004 b7
dr-xr-xr-x  3 root root 512 Jun 25  2004 b8
dr-xr-xr-x  3 root root 512 Jun 25  2004 b9

./test/a3/b3:
total 0

./test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b4/toto:
total 0

./test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b5/toto:
total 0

./test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b6/toto:
total 0

./test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b7/toto:
total 0

./test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b8/toto:
total 0

./test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./test/a3/b9/toto:
total 0

./zetest:
total 1
lr--r--r--  1 root root   7 Jan  1  2000 link_to_test -> ../test
dr-xr-xr-x  5 root root 512 Jan  1  2000 test

./zetest/test:
total 2
dr-xr-xr-x  2 root root 512 Jan  1  2000 a1
dr-xr-xr-x  4 root root 512 Jan  1  2000 a2
dr-xr-xr-x  9 root root 512 Jan  1  2000 a3

./zetest/test/a1:
total 0

./zetest/test/a2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 b1
dr-xr-xr-x  3 root root 512 Jan  1  2000 b2

./zetest/test/a2/b1:
total 1
-r--r--r--  1 root root 6 Jun 25  2004 f1

./zetest/test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 c1

./zetest/test/a2/b2/c1:
total 0

./zetest/test/a3:
total 4
dr-xr-xr-x  2 root root 512 Jan  1  2000 b3
dr-xr-xr-x  3 root root 512 Jun 25  2004 b4
dr-xr-xr-x  3 root root 512 Jun 25  2004 b5
dr-xr-xr-x  3 root root 512 Jun 25  2004 b6
dr-xr-xr-x  3 root root 512 Jun 25  2004 b7
dr-xr-xr-x  3 root root 512 Jun 25  2004 b8
dr-xr-xr-x  3 root root 512 Jun 25  2004 b9

./zetest/test/a3/b3:
total 0

./zetest/test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b4/toto:
total 0

./zetest/test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b5/toto:
total 0

./zetest/test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b6/toto:
total 0

./zetest/test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b7/toto:
total 0

./zetest/test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b8/toto:
total 0

./zetest/test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 Jan  1  2000 toto

./zetest/test/a3/b9/toto:
total 0
EOF


diff -u - <(stat --format="%n %A s=%s b=%b l=%h at='%x' mt='%y' ct='%z'" ./test/a3/* ./atest/link_to_test ./test/a2/b1/f1 ) <<-EOF || exit 1 
./test/a3/b3 dr-xr-xr-x s=512 b=1 l=2 at='2000-01-01 12:00:00.000000000 +0100' mt='2000-01-01 12:00:00.000000000 +0100' ct='2000-01-01 12:00:00.000000000 +0100'
./test/a3/b4 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 04:03:00.000000000 +0200' mt='2004-06-25 04:03:00.000000000 +0200' ct='2004-06-25 04:03:00.000000000 +0200'
./test/a3/b5 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 05:04:00.000000000 +0200' mt='2004-06-25 05:04:00.000000000 +0200' ct='2004-06-25 05:04:00.000000000 +0200'
./test/a3/b6 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 06:05:00.000000000 +0200' mt='2004-06-25 06:05:00.000000000 +0200' ct='2004-06-25 06:05:00.000000000 +0200'
./test/a3/b7 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 07:06:00.000000000 +0200' mt='2004-06-25 07:06:00.000000000 +0200' ct='2004-06-25 07:06:00.000000000 +0200'
./test/a3/b8 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 08:07:00.000000000 +0200' mt='2004-06-25 08:07:00.000000000 +0200' ct='2004-06-25 08:07:00.000000000 +0200'
./test/a3/b9 dr-xr-xr-x s=512 b=1 l=3 at='2004-06-25 09:08:00.000000000 +0200' mt='2004-06-25 09:08:00.000000000 +0200' ct='2004-06-25 09:08:00.000000000 +0200'
./atest/link_to_test lr--r--r-- s=4 b=1 l=1 at='2000-01-01 12:00:00.000000000 +0100' mt='2000-01-01 12:00:00.000000000 +0100' ct='2000-01-01 12:00:00.000000000 +0100'
./test/a2/b1/f1 -r--r--r-- s=6 b=1 l=1 at='2004-06-25 15:00:00.000000000 +0200' mt='2004-06-25 15:00:00.000000000 +0200' ct='2004-06-25 15:00:00.000000000 +0200'
EOF


cd test/a2 || exit 1

ls b1 > /dev/null || exit 1

# check there is no "leak" between directories
for d in a1 a3 b3 b4 b5 b6 b7 b8 b9 toto c1 f1 test atest zetest
do
    ls $d >& /dev/null && exit 1
done


exit 0

