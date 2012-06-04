#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import sys, string, unittest

#from twisted.internet import defer, threads
#from twisted.trial import unittest


class TestLevelDB(unittest.TestCase):
	def setUp(self):
		# import local leveldb
		import leveldb as _leveldb
		self.leveldb = _leveldb

		# Python2/3 compat
		if hasattr(string, 'lowercase'):
			self.lowercase = string.lowercase
			self.uppercase = string.uppercase
		else:
			self.lowercase = string.ascii_lowercase
			self.uppercase = string.ascii_uppercase

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
		db.Put(self._s('a'), self._s('b'))
		i = db.RangeIter(include_value = False, reverse = True)
		#del self.leveldb

	def _s(self, s):
		if sys.version_info[0] >= 3:
			return bytearray(s, encoding = 'latin1')
		else:
			return s

	def _join(self, i):
		return self._s('').join(i)

	# NOTE: modeled after test 'Snapshot'
	def testSnapshotBasic(self):
		db = self._open()

		# destroy database, if any
		db.Put(self._s('foo'), self._s('v1'))
		s1 = db.CreateSnapshot()

		db.Put(self._s('foo'), self._s('v2'))
		s2 = db.CreateSnapshot()

		db.Put(self._s('foo'), self._s('v3'))
		s3 = db.CreateSnapshot()

		db.Put(self._s('foo'), self._s('v4'))

		self.assertEquals(s1.Get(self._s('foo')), self._s('v1'))
		self.assertEquals(s2.Get(self._s('foo')), self._s('v2'))
		self.assertEquals(s3.Get(self._s('foo')), self._s('v3'))
		self.assertEquals(db.Get(self._s('foo')), self._s('v4'))

		# TBD: close properly
		del s3
		self.assertEquals(s1.Get(self._s('foo')), self._s('v1'))
		self.assertEquals(s2.Get(self._s('foo')), self._s('v2'))
		self.assertEquals(db.Get(self._s('foo')), self._s('v4'))

		# TBD: close properly
		del s1
		self.assertEquals(s2.Get(self._s('foo')), self._s('v2'))
		self.assertEquals(db.Get(self._s('foo')), self._s('v4'))

		# TBD: close properly
		del s2
		self.assertEquals(db.Get(self._s('foo')), self._s('v4'))

		# re-open
		del db
		db = self._open()
		self.assertEquals(db.Get(self._s('foo')), self._s('v4'))

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

		for c in self.lowercase:
			b.Put(self._s(c), self._s('hello'))

		db.Write(b)

	def _insert_uppercase_batch(self, db):
		b = self.leveldb.WriteBatch()

		for c in self.uppercase:
			b.Put(self._s(c), self._s('hello'))

		db.Write(b)

	def _test_uppercase_get(self, db):
		for k in self.uppercase:
			v = db.Get(self._s(k))
			self.assertEquals(v, self._s('hello'))
			self.assert_(k in self.uppercase)

	def _test_uppercase_iter(self, db):
		s = self._join(k for k, v in db.RangeIter(self._s('J'), self._s('M')))
		self.assertEquals(s, self._s('JKLM'))

		s = self._join(k for k, v in db.RangeIter(self._s('S')))
		self.assertEquals(s, self._s('STUVWXYZ'))

		s = self._join(k for k, v in db.RangeIter(key_to = self._s('E')))
		self.assertEquals(s, self._s('ABCDE'))

	def _test_uppercase_iter_rev(self, db):
		# inside range
		s = self._join(k for k, v in db.RangeIter(self._s('J'), self._s('M'), reverse = True))
		self.assertEquals(s, self._s('MLKJ'))

		# partly outside range
		s = self._join(k for k, v in db.RangeIter(self._s('Z'), self._s(chr(ord('Z') + 1)), reverse = True))
		self.assertEquals(s, self._s('Z'))
		s = self._join(k for k, v in db.RangeIter(self._s(chr(ord('A') - 1)), self._s('A'), reverse = True))
		self.assertEquals(s, self._s('A'))

		# wholly outside range
		s = self._join(k for k, v in db.RangeIter(self._s(chr(ord('Z') + 1)), self._s(chr(ord('Z') + 2)), reverse = True))
		self.assertEquals(s, self._s(''))

		s = self._join(k for k, v in db.RangeIter(self._s(chr(ord('A') - 2)), self._s(chr(ord('A') - 1)), reverse = True))
		self.assertEquals(s, self._s(''))

		# lower limit
		s = self._join(k for k, v in db.RangeIter(self._s('S'), reverse = True))
		self.assertEquals(s, self._s('ZYXWVUTS'))

		# upper limit
		s = self._join(k for k, v in db.RangeIter(key_to = self._s('E'), reverse = True))
		self.assertEquals(s, self._s('EDCBA'))

	def _test_lowercase_iter(self, db):
		s = self._join(k for k, v in db.RangeIter(self._s('j'), self._s('m')))
		self.assertEquals(s, self._s('jklm'))

		s = self._join(k for k, v in db.RangeIter(self._s('s')))
		self.assertEquals(s, self._s('stuvwxyz'))

		s = self._join(k for k, v in db.RangeIter(key_to = self._s('e')))
		self.assertEquals(s, self._s('abcde'))

	def _test_lowercase_iter(self, db):
		s = self._join(k for k, v in db.RangeIter(self._s('j'), self._s('m'), reverse = True))
		self.assertEquals(s, self._s('mlkj'))

		s = self._join(k for k, v in db.RangeIter(self._s('s'), reverse = True))
		self.assertEquals(s, self._s('zyxwvuts'))

		s = self._join(k for k, v in db.RangeIter(key_to = self._s('e'), reverse = True))
		self.assertEquals(s, self._s('edcba'))

	def _test_lowercase_get(self, db):
		for k in self.lowercase:
			v = db.Get(self._s(k))
			self.assertEquals(v, self._s('hello'))
			self.assert_(k in self.lowercase)

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
		db.Put(self._s('key1'), self._s('val1'))
		del db
		db = self._open()
		db.Delete(self._s('key2'))
		db.Delete(self._s('key1'))
		del db
		db = self._open()
		db.Delete(self._s('key2'))
		del db
		db = self._open()
		db.Put(self._s('key3'), self._s('val1'))
		del db
		db = self._open()
		del db
		db = self._open()
		v = list(db.RangeIter())
		self.assertEquals(v, [(self._s('key3'), self._s('val1'))])

	def testOpenSame(self):
		a = self._open()
		a.Put(self._s('foo'), self._s('bar'))

		# this one should fail, we need to fix this in the bindings
		b = self._open()

if __name__ == '__main__':
	unittest.main()