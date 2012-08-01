#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import leveldb, pointless, random, string, itertools, collections

from twisted.internet import reactor, defer, threads

def compare(a, b):
	return cmp(a, b)

c = 'bytewise'
c = ('leveldb.BytewiseComparator', compare)

kw = {
	'create_if_missing': True,
	'error_if_exists': False,
	'paranoid_checks': False,
	'block_cache_size': 8 * (2 << 20),
	'write_buffer_size': 2 * (2 << 20),
	'block_size': 4096,
	'max_open_files': 1000,
	'block_restart_interval': 16,
	'comparator': c
}

def random_value(n):
	return bytearray(''.join(random.choice(string.ascii_letters) for i in xrange(n)))

def generate_data():
	random.seed(0)
	k_ = []
	v_ = []

	for i in xrange(1000000):
		k = random_value(8)
		v = random_value(8)
		k_.append(k)
		v_.append(v)

	pointless.serialize([k_, v_], 'data.map')

@defer.inlineCallbacks
def insert_alot(db, kv, ops, stop):
	while not stop[0]:
		k = random.choice(kv[0])
		v = random.choice(kv[1])
		yield threads.deferToThread(db.Put, k, v)

		ops['n_insert'] += 1

		if ops['n_insert'] > 0 and ops['n_insert'] % 1000 == 0:
			print 'INFO: n_insert: %iK' % (ops['n_insert'] // 1000,)

@defer.inlineCallbacks
def scan_alot(db, kv, ops, stop):
	n_scans = 0

	while not stop[0]:
		k_a = random.choice(kv[0])
		k_b = random.choice(kv[0])
		k_a, k_b = min(k_a, k_b), min(k_a, k_b)
		i = db.RangeIter(k_a, k_b)
		n_max = random.randint(100, 10000)

		for c in itertools.count():
			try:
				next = yield threads.deferToThread(i.next)
			except StopIteration:
				break

			if c > n_max:
				break

		ops['n_scans'] += 1

		if ops['n_scans'] > 0 and ops['n_scans'] % 1000 == 0:
			print 'INFO: n_scans: %iK' % (ops['n_scans'] // 1000,)

def main():
	#generate_data()

	reactor.suggestThreadPoolSize(20)

	kv = pointless.Pointless('/home/arni/py-leveldb/data.map', allow_print = False).GetRoot()
	db = leveldb.LevelDB('./db', **kw)

	stop = [False]

	def do_stop():
		stop[0] = True
		reactor.stop()

	ops = collections.defaultdict(int)

	for i in xrange(10):
		reactor.callWhenRunning(insert_alot, db, kv, ops, stop)
		reactor.callWhenRunning(scan_alot, db, kv, ops, stop)

	reactor.callLater(10000.0, do_stop)

	reactor.run()


if __name__ == '__main__':
	main()
