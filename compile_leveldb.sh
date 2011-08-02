#!/bin/sh

(cd leveldb-read-only; make OPT='-fPIC -O2 -DNDEBUG')
