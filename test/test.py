#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import sys, string

def print_usage_exit():
	sys.exit('usage: ./test.py db_dir import_dir')

def main():
	if len(sys.argv) != 3:
		print_usage_exit()

	db_dir = sys.argv[1]
	import_dir = sys.argv[2]

	p = sys.path
	sys.path = [import_dir]
	import leveldb
	sys.path = p

	db = leveldb.LevelDB(db_dir)

	# first test
	clear(db)
	insert_lowercase(db)
	assert(len(list(db.RangeIter())) == 26)
	test_lowercase_iter(db)
	test_lowercase_get(db)

	# clear databse
	clear_batch(leveldb.WriteBatch(), db)
	insert_uppercase_batch(leveldb.WriteBatch(), db)
	test_uppercase_iter(db)
	test_uppercase_get(db)
	assert(len(list(db.RangeIter())) == 26)

def insert_lowercase(db):
	for c in string.lowercase:
		db.Put(c, 'hello')

def test_lowercase_get(db):
	for k in string.lowercase:
		v = db.Get(k)
		assert(v == 'hello')
		assert(k in string.lowercase)

def test_lowercase_iter(db):
	s = ''.join(k for k, v in db.RangeIter('j', 'm'))
	assert(s == 'jklm')

	s = ''.join(k for k, v in db.RangeIter('s'))
	assert(s == 'stuvwxyz')

	s = ''.join(k for k, v in db.RangeIter(key_to = 'e'))
	assert(s == 'abcde')

def clear(db):
	for k in db.RangeIter(include_value = False):
		db.Delete(k)

def clear_batch(batch, db):
	for k in db.RangeIter(include_value = False):
		batch.Delete(k)

	db.Write(batch)

def insert_uppercase_batch(batch, db):
	for c in string.uppercase:
		batch.Put(c, 'hello')

	db.Write(batch)

def test_uppercase_iter(db):
	s = ''.join(k for k, v in db.RangeIter('J', 'M'))
	assert(s == 'JKLM')

	s = ''.join(k for k, v in db.RangeIter('S'))
	assert(s == 'STUVWXYZ')

	s = ''.join(k for k, v in db.RangeIter(key_to = 'E'))
	assert(s == 'ABCDE')

def test_uppercase_get(db):
	for k in string.uppercase:
		v = db.Get(k)
		assert(v == 'hello')
		assert(k in string.uppercase)

if __name__ == '__main__':
	main()

#db = leveldb.LevelDB('./db')
#db.Put('a', 'b' * 100, sync = False)
#print db.Get('a')
#del db
#leveldb.RepairDB('./db')