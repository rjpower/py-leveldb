#!/usr/bin/python

import sys

p = sys.path
sys.path = ['/home/arni/py-leveldb/build/lib.linux-x86_64-2.7']
import leveldb
sys.path = p

print leveldb
