#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import sys, string

from twisted.internet import defer, threads
from twisted.trial import unittest

class TestLevelDB(unittest.TestCase):
	def setUp(self):
		# import local leveldb
		p = sys.path

		try:
			sys.path = ['/home/arni/code/py-leveldb/build/lib.linux-x86_64-2.7']
			import leveldb as _leveldb
			self.leveldb = _leveldb
		finally:
			sys.path = p

		# destroy previous database, if any
		self.name = 'db_a'
		self.leveldb.DestroyDB(self.name)

	def _open_options(self, create_if_missing = True, error_if_exists = False):
		return {
			'create_if_missing': True,
			'error_if_exists': error_if_exists,
			'paranoid_checks': False,
			'block_cache_size': 8 * (2 << 20),
			'write_buffer_size': 2 * (2 << 20),
			'block_size': 4096,
			'max_open_files': 1000,
			'block_restart_interval': 16
		}

	def _open(self, *args, **kwargs):
		options = self._open_options(*args, **kwargs)
		return self.leveldb.LevelDB(self.name, **options)

	def testIteratorCrash(self):
		options = self._open_options()
		db = self.leveldb.LevelDB(self.name, **options)
		db.Put('a', 'b')
		i = db.RangeIter(include_value = False, reverse = True)
		#del self.leveldb

	# NOTE: modeled after test 'Snapshot'
	def testSnapshotBasic(self):
		db = self._open()

		# destroy database, if any
		db.Put('foo', 'v1')
		s1 = db.CreateSnapshot()

		db.Put('foo', 'v2')
		s2 = db.CreateSnapshot()

		db.Put('foo', 'v3')
		s3 = db.CreateSnapshot()

		db.Put('foo', 'v4')

		self.assertEquals(s1.Get('foo'), 'v1')
		self.assertEquals(s2.Get('foo'), 'v2')
		self.assertEquals(s3.Get('foo'), 'v3')
		self.assertEquals(db.Get('foo'), 'v4')

		# TBD: close properly
		del s3
		self.assertEquals(s1.Get('foo'), 'v1')
		self.assertEquals(s2.Get('foo'), 'v2')
		self.assertEquals(db.Get('foo'), 'v4')

		# TBD: close properly
		del s1
		self.assertEquals(s2.Get('foo'), 'v2')
		self.assertEquals(db.Get('foo'), 'v4')

		# TBD: close properly
		del s2
		self.assertEquals(db.Get('foo'), 'v4')

		# re-open
		del db
		db = self._open()
		self.assertEquals(db.Get('foo'), 'v4')

	def ClearDB(self, db):
		for k in list(db.RangeIter(include_value = False, reverse = True)):
			db.Delete(k)

	def ClearDB_batch(self, db):
		b = self.leveldb.WriteBatch()

		for k in db.RangeIter(include_value = False, reverse = True):
			b.Delete(k)

		db.Write(b)

	def CountDB(self, db):
		return sum(1 for i in db.RangeIter(reverse = True))

	def _insert_lowercase(self, db):
		b = self.leveldb.WriteBatch()

		for c in string.lowercase:
			b.Put(c, 'hello')

		db.Write(b)

	def _insert_uppercase_batch(self, db):
		b = self.leveldb.WriteBatch()

		for c in string.uppercase:
			b.Put(c, 'hello')

		db.Write(b)

	def _test_uppercase_get(self, db):
		for k in string.uppercase:
			v = db.Get(k)
			self.assertEquals(v, 'hello')
			self.assert_(k in string.uppercase)

	def _test_uppercase_iter(self, db):
		s = ''.join(k for k, v in db.RangeIter('J', 'M'))
		self.assertEquals(s, 'JKLM')

		s = ''.join(k for k, v in db.RangeIter('S'))
		self.assertEquals(s, 'STUVWXYZ')

		s = ''.join(k for k, v in db.RangeIter(key_to = 'E'))
		self.assertEquals(s, 'ABCDE')

	def _test_uppercase_iter_rev(self, db):
		# inside range
		s = ''.join(k for k, v in db.RangeIter('J', 'M', reverse = True))
		self.assertEquals(s, 'MLKJ')

		# partly outside range
		s = ''.join(k for k, v in db.RangeIter('Z', chr(ord('Z') + 1), reverse = True))
		self.assertEquals(s, 'Z')
		s = ''.join(k for k, v in db.RangeIter(chr(ord('A') - 1), 'A', reverse = True))
		self.assertEquals(s, 'A')

		# wholly outside range
		s = ''.join(k for k, v in db.RangeIter(chr(ord('Z') + 1), chr(ord('Z') + 2), reverse = True))
		self.assertEquals(s, '')

		s = ''.join(k for k, v in db.RangeIter(chr(ord('A') - 2), chr(ord('A') - 1), reverse = True))
		self.assertEquals(s, '')

		# lower limit
		s = ''.join(k for k, v in db.RangeIter('S', reverse = True))
		self.assertEquals(s, 'ZYXWVUTS')

		# upper limit
		s = ''.join(k for k, v in db.RangeIter(key_to = 'E', reverse = True))
		self.assertEquals(s, 'EDCBA')

	def _test_lowercase_iter(self, db):
		s = ''.join(k for k, v in db.RangeIter('j', 'm'))
		self.assertEquals(s, 'jklm')

		s = ''.join(k for k, v in db.RangeIter('s'))
		self.assertEquals(s, 'stuvwxyz')

		s = ''.join(k for k, v in db.RangeIter(key_to = 'e'))
		self.assertEquals(s, 'abcde')

	def _test_lowercase_iter(self, db):
		s = ''.join(k for k, v in db.RangeIter('j', 'm', reverse = True))
		self.assertEquals(s, 'mlkj')

		s = ''.join(k for k, v in db.RangeIter('s', reverse = True))
		self.assertEquals(s, 'zyxwvuts')

		s = ''.join(k for k, v in db.RangeIter(key_to = 'e', reverse = True))
		self.assertEquals(s, 'edcba')

	def _test_lowercase_get(self, db):
		for k in string.lowercase:
			v = db.Get(k)
			self.assertEquals(v, 'hello')
			self.assert_(k in string.lowercase)

	def testIterationBasic(self):
		db = self._open()
		self._insert_lowercase(db)
		self.assertEquals(self.CountDB(db), 26)
		self._test_lowercase_iter(db)
		#self._test_lowercase_iter_rev(db)
		self._test_lowercase_get(db)
		self.ClearDB_batch(db)
		self._insert_uppercase_batch(db)
		self._test_uppercase_iter(db)
		self._test_uppercase_iter_rev(db)
		self._test_uppercase_get(db)
		self.assertEquals(self.CountDB(db), 26)

	# tried to re-produce http://code.google.com/p/leveldb/issues/detail?id=44
	def testMe(self):
		db = self._open()
		db.Put('key1', 'val1')
		del db
		db = self._open()
		db.Delete('key2')
		db.Delete('key1')
		del db
		db = self._open()
		db.Delete('key2')
		del db
		db = self._open()
		db.Put('key3', 'val1')
		del db
		db = self._open()
		del db
		db = self._open()
		v = list(db.RangeIter())
		self.assertEquals(v, [('key3', 'val1')])

	def testOpenSame(self):
		a = self._open()
		a.Put('foo', 'bar')

		# this one should fail, we need to fix this in the bindings
		b = self._open()
