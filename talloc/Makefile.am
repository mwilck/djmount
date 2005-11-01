# $Id$
#
# Top-level "Makefile.am" for talloc inside djmount
#
# This file replaces the standard talloc make procedure 
# (see talloc/Makefile.in) which does not support all automake features
# e.g. "make dist"
#



noinst_LIBRARIES	= libtalloc.a
noinst_HEADERS		= talloc/talloc.h
libtalloc_a_SOURCES	= talloc/talloc.c

# distribute only talloc files necessary to djmount build or developments
EXTRA_DIST = talloc/config.m4 talloc/talloc.3 talloc/talloc.3.xml \
		talloc/talloc_guide.txt

check_PROGRAMS		= testsuite
TESTS			= testsuite

testsuite_SOURCES 	= talloc/testsuite.c
testsuite_LDADD		= -ltalloc
testsuite_LDFLAGS	= -L.


