#ifndef __LEVELDB__MODULE__H__
#define __LEVELDB__MODULE__H__

extern "C" {
#include <Python.h>

#include "structmember.h"

#include <stdio.h>
#include <limits.h>
}

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/comparator.h>
#include <leveldb/cache.h>

#include <vector>

#define LEVELDB_FUNC_N_DOC(func) PyObject* func(PyObject* o, PyObject* args); extern const char func##_doc[]
#define LEVELDB_FUNC_DEF(pyfunc, func) { pyfunc, func, METH_VARARGS, func##_doc}

LEVELDB_FUNC_N_DOC(leveldb_repair_db);

typedef struct {
	PyObject_HEAD
	leveldb::DB* _db;
	leveldb::Options* _options;
	leveldb::Cache* _cache;
} PyLevelDB;

typedef struct {
	bool is_put;
	std::string key;
	std::string value;
} PyWriteBatchEntry;

typedef struct {
	PyObject_HEAD
	std::vector<PyWriteBatchEntry>* ops;
} PyWriteBatch;

// custom types
extern PyTypeObject PyLevelDBType;
extern PyTypeObject PyWriteBatchType;

#define PyLevelDB_Check(op) PyObject_TypeCheck(op, &PyLevelDBType)
#define PyWriteBatch_Check(op) PyObject_TypeCheck(op, &PyWriteBatchType)

#endif
