#!/bin/bash



mntpoint=$( mktemp -d ${TMPDIR:-/tmp}/test_vfs.XXXXXX ) || exit 1

# Delete temporaries on exit, hangup, interrupt, quit, termination
trap "cd / ; fusermount -u $mntpoint 2> /dev/null ; rmdir $mntpoint" EXIT HUP INT QUIT TERM

./test_vfs $mntpoint || exit 1


cd $mntpoint || exit 1

diff - <(find . -print) <<-EOF || exit 1 
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
	./.debug
	./.debug/talloc_total
	./.debug/talloc_report
	./.debug/talloc_report_full
EOF


diff - <(/bin/ls -lR) <<-EOF || exit 1 
.:
total 2
dr-xr-xr-x  3 root root 512 jan  1  2000 atest
dr-xr-xr-x  5 root root 512 jan  1  2000 test
dr-xr-xr-x  3 root root 512 jan  1  2000 zetest

./atest:
total 1
dr-xr-xr-x  5 root root 512 jan  1  2000 test

./atest/test:
total 2
dr-xr-xr-x  2 root root 512 jan  1  2000 a1
dr-xr-xr-x  4 root root 512 jan  1  2000 a2
dr-xr-xr-x  9 root root 512 jan  1  2000 a3

./atest/test/a1:
total 0

./atest/test/a2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 b1
dr-xr-xr-x  3 root root 512 jan  1  2000 b2

./atest/test/a2/b1:
total 1
-r--r--r--  1 root root 6 jan  1  2000 f1

./atest/test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 c1

./atest/test/a2/b2/c1:
total 0

./atest/test/a3:
total 4
dr-xr-xr-x  2 root root 512 jan  1  2000 b3
dr-xr-xr-x  3 root root 512 jan  1  2000 b4
dr-xr-xr-x  3 root root 512 jan  1  2000 b5
dr-xr-xr-x  3 root root 512 jan  1  2000 b6
dr-xr-xr-x  3 root root 512 jan  1  2000 b7
dr-xr-xr-x  3 root root 512 jan  1  2000 b8
dr-xr-xr-x  3 root root 512 jan  1  2000 b9

./atest/test/a3/b3:
total 0

./atest/test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b4/toto:
total 0

./atest/test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b5/toto:
total 0

./atest/test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b6/toto:
total 0

./atest/test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b7/toto:
total 0

./atest/test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b8/toto:
total 0

./atest/test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./atest/test/a3/b9/toto:
total 0

./test:
total 2
dr-xr-xr-x  2 root root 512 jan  1  2000 a1
dr-xr-xr-x  4 root root 512 jan  1  2000 a2
dr-xr-xr-x  9 root root 512 jan  1  2000 a3

./test/a1:
total 0

./test/a2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 b1
dr-xr-xr-x  3 root root 512 jan  1  2000 b2

./test/a2/b1:
total 1
-r--r--r--  1 root root 6 jan  1  2000 f1

./test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 c1

./test/a2/b2/c1:
total 0

./test/a3:
total 4
dr-xr-xr-x  2 root root 512 jan  1  2000 b3
dr-xr-xr-x  3 root root 512 jan  1  2000 b4
dr-xr-xr-x  3 root root 512 jan  1  2000 b5
dr-xr-xr-x  3 root root 512 jan  1  2000 b6
dr-xr-xr-x  3 root root 512 jan  1  2000 b7
dr-xr-xr-x  3 root root 512 jan  1  2000 b8
dr-xr-xr-x  3 root root 512 jan  1  2000 b9

./test/a3/b3:
total 0

./test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b4/toto:
total 0

./test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b5/toto:
total 0

./test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b6/toto:
total 0

./test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b7/toto:
total 0

./test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b8/toto:
total 0

./test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./test/a3/b9/toto:
total 0

./zetest:
total 1
dr-xr-xr-x  5 root root 512 jan  1  2000 test

./zetest/test:
total 2
dr-xr-xr-x  2 root root 512 jan  1  2000 a1
dr-xr-xr-x  4 root root 512 jan  1  2000 a2
dr-xr-xr-x  9 root root 512 jan  1  2000 a3

./zetest/test/a1:
total 0

./zetest/test/a2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 b1
dr-xr-xr-x  3 root root 512 jan  1  2000 b2

./zetest/test/a2/b1:
total 1
-r--r--r--  1 root root 6 jan  1  2000 f1

./zetest/test/a2/b2:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 c1

./zetest/test/a2/b2/c1:
total 0

./zetest/test/a3:
total 4
dr-xr-xr-x  2 root root 512 jan  1  2000 b3
dr-xr-xr-x  3 root root 512 jan  1  2000 b4
dr-xr-xr-x  3 root root 512 jan  1  2000 b5
dr-xr-xr-x  3 root root 512 jan  1  2000 b6
dr-xr-xr-x  3 root root 512 jan  1  2000 b7
dr-xr-xr-x  3 root root 512 jan  1  2000 b8
dr-xr-xr-x  3 root root 512 jan  1  2000 b9

./zetest/test/a3/b3:
total 0

./zetest/test/a3/b4:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b4/toto:
total 0

./zetest/test/a3/b5:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b5/toto:
total 0

./zetest/test/a3/b6:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b6/toto:
total 0

./zetest/test/a3/b7:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b7/toto:
total 0

./zetest/test/a3/b8:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b8/toto:
total 0

./zetest/test/a3/b9:
total 1
dr-xr-xr-x  2 root root 512 jan  1  2000 toto

./zetest/test/a3/b9/toto:
total 0
EOF


cd test/a2 || exit 1

ls b1 > /dev/null || exit 1

ls a3 >& /dev/null && exit 1


exit 0

