#! /bin/sh

export CXX=/opt/canmore/IntelCE/bin/i686-cm-linux-g++
export CC=/opt/canmore/IntelCE/bin/i686-cm-linux-gcc

./configure CPPFLAGS=-I/opt/canmore/local/include LDFLAGS=-L/opt/canmore/local/lib
