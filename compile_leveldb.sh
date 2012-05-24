#!/bin/sh

set -e;

(
	svn checkout http://snappy.googlecode.com/svn/trunk/ snappy-read-only;
	cd snappy-read-only;
	./autogen.sh;
	./configure --enable-shared=no --enable-static=yes;
	make clean;
	make CXXFLAGS='-g -O2 -fPIC';
)

(
	git clone https://code.google.com/p/leveldb/ || (cd leveldb; git pull);
	cd leveldb;
	make clean;
	make OPT='-fPIC -O2 -DNDEBUG -DSNAPPY -I../snappy-read-only' SNAPPY_CFLAGS=''
)
