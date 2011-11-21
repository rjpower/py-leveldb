#!/bin/sh

(
	cd snappy-read-only;
	./autogen.sh;
	./configure --enable-shared=no --enable-static=yes;
	make clean;
	make CXXFLAGS='-g -O2 -fPIC';
)

(
	cd leveldb-read-only;
	make clean;
	make OPT='-fPIC -O2 -DNDEBUG -DSNAPPY -I../snappy-read-only' SNAPPY_CFLAGS=''
)
