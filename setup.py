#!/usr/bin/python

# Copyright (c) Arni Mar Jonsson.
# See LICENSE for details.

import sys
from distutils.core import setup, Extension

extra_compile_args = ['-I./leveldb-read-only/include', '-fPIC', '--std=c++0x', '-pedantic', '-Wall', '-g2', '-D_GNU_SOURCE', '-O2', '-DNDEBUG']
extra_link_args = ['-L./leveldb-read-only', '-Bstatic', '-lleveldb']

setup(
	name = 'leveldb',
	version = '0.1',
	maintainer = 'Arni Mar Jonsson',
	maintainer_email = 'arnimarj@gmail.com',
	url = 'http://code.google.com/p/py-leveldb/',

	classifiers = [
		'Development Status :: 4 - Beta',
		'Environment :: Other Environment',
		'Intended Audience :: Developers',
		'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
		'Operating System :: POSIX',
		'Programming Language :: C',
		'Programming Language :: Python',
		'Programming Language :: Python :: 2.4',
		'Programming Language :: Python :: 2.5',
		'Programming Language :: Python :: 2.6',
		'Programming Language :: Python :: 2.7',
		'Topic :: Database',
		'Topic :: Software Development :: Libraries'
	],

	#download_url = 'http://py-leveldb.googlecode.com/files/leveldb.-0.1.tar.gz',
	description = 'Python bindings for leveldb database library',
	# long_description = 

	packages = ['leveldb'],
	package_dir = {'leveldb': ''},

	ext_modules = [
		Extension('leveldb',
			sources = [
				# python stuff
				'leveldb_ext.cc',
				'leveldb_object.cc',
			],
			libraries = ['stdc++'],

			extra_compile_args = extra_compile_args,
			extra_link_args = extra_link_args
		)
	]
)
