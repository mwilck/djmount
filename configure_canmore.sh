#! /bin/sh

export CXX=/opt/canmore/IntelCE/bin/i686-cm-linux-g++
export CC=/opt/canmore/IntelCE/bin/i686-cm-linux-gcc
export CPPFLAGS=-I/opt/canmore/local/include 
export LDFLAGS="-L/opt/canmore/local/lib -liconv"
./configure --host=i686-cm-linux --prefix=/opt/canmore/local
