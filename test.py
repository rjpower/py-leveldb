#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import sys, time

p = sys.path
sys.path = ['/home/arni/py-leveldb/build/lib.linux-x86_64-2.7']
import leveldb
sys.path = p

print leveldb
db = leveldb.LevelDB('./db')
db.Put('a', 'b' * 100, sync = False)

print db.Get('a')

#del db
#leveldb.RepairDB('./db')