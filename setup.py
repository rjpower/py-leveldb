#!/usr/bin/env python

# Copyright (c) Arni Mar Jonsson.
#
# Updates to setup.py/PyPi - Russell Power (power@cs.nyu.edu)
#
#
# See LICENSE for details.

import platform
import sys

from setuptools import setup, Extension

system, node, release, version, machine, processor = platform.uname()
common_flags = [
    '-I./leveldb/include',
    '-I./leveldb',
    '-I./snappy',
    '-I.',
    '-fno-builtin-memcmp',
    '-O2',
    '-fPIC',
    '-DNDEBUG',
    '-DSNAPPY',
]

if system == 'Darwin':
    extra_compile_args = common_flags + [
        '-DOS_MACOSX',
        '-DLEVELDB_PLATFORM_POSIX',
        '-Wno-error=unused-command-line-argument-hard-error-in-future',
    ]
elif system == 'Linux':
    extra_compile_args = common_flags + [
        '-pthread',
        '-Wall',
        '-DOS_LINUX',
        '-DLEVELDB_PLATFORM_POSIX',
    ]
else:
    sys.stderr.write("Don't know how to compile leveldb for %s!\n" % system)
    sys.exit(1)

setup(
    name = 'leveldb',
    version = '0.20',
    maintainer = 'Arni Mar Jonsson',
    maintainer_email = 'arnimarkj@gmail.com',
    url = 'https://code.google.com/p/py-leveldb/',

    classifiers = [
        'Development Status :: 4 - Beta',
        'Environment :: Other Environment',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Operating System :: POSIX',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.4',
        'Programming Language :: Python :: 2.5',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.0',
        'Programming Language :: Python :: 3.1',
        'Programming Language :: Python :: 3.2',
        'Programming Language :: Python :: 3.3',
        'Topic :: Database',
        'Topic :: Software Development :: Libraries'
    ],

    description = 'Python bindings for leveldb database library',

    ext_modules = [
        Extension('leveldb',
            sources = [
                # snappy
                './snappy/snappy.cc',
                './snappy/snappy-stubs-internal.cc',
                './snappy/snappy-sinksource.cc',
                './snappy/snappy-c.cc',

                #leveldb
                'leveldb/db/builder.cc',
                'leveldb/db/c.cc',
                'leveldb/db/db_impl.cc',
                'leveldb/db/db_iter.cc',
                'leveldb/db/dbformat.cc',
                'leveldb/db/filename.cc',
                'leveldb/db/log_reader.cc',
                'leveldb/db/log_writer.cc',
                'leveldb/db/memtable.cc',
                'leveldb/db/repair.cc',
                'leveldb/db/table_cache.cc',
                'leveldb/db/version_edit.cc',
                'leveldb/db/version_set.cc',
                'leveldb/db/write_batch.cc',
                'leveldb/table/block.cc',
                'leveldb/table/block_builder.cc',
                'leveldb/table/filter_block.cc',
                'leveldb/table/format.cc',
                'leveldb/table/iterator.cc',
                'leveldb/table/merger.cc',
                'leveldb/table/table.cc',
                'leveldb/table/table_builder.cc',
                'leveldb/table/two_level_iterator.cc',
                'leveldb/util/arena.cc',
                'leveldb/util/bloom.cc',
                'leveldb/util/cache.cc',
                'leveldb/util/coding.cc',
                'leveldb/util/comparator.cc',
                'leveldb/util/crc32c.cc',
                'leveldb/util/env.cc',
                'leveldb/util/env_posix.cc',
                'leveldb/util/filter_policy.cc',
                'leveldb/util/hash.cc',
                'leveldb/util/histogram.cc',
                'leveldb/util/logging.cc',
                'leveldb/util/options.cc',
                'leveldb/util/status.cc',
                'leveldb/port/port_posix.cc',

                # python stuff
                'leveldb_ext.cc',
                'leveldb_object.cc',
            ],
            libraries = ['stdc++'],
            extra_compile_args = extra_compile_args,
        )
    ]
)
