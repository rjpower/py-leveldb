#ifndef __LEVELDB__MODULE__H__
#define __LEVELDB__MODULE__H__

extern "C" {
#include <Python.h>

#include "structmember.h"

#include <stdio.h>
#include <limits.h>
}

#include <leveldb/db.h>

#define LEVELDB_FUNC_N_DOC(func) PyObject* func(PyObject* o, PyObject* args); extern const char func##_doc[]
#define LEVELDB_FUNC_DEF(pyfunc, func) { pyfunc, func, METH_VARARGS, func##_doc}

LEVELDB_FUNC_N_DOC(leveldb_hello_world);

typedef struct {
	PyObject_HEAD
	leveldb::DB* db;
} PyLevelDB;

// custom types
extern PyTypeObject PyLevelDBType;

#define PyLevelDB_Check(op) PyObject_TypeCheck(op, &PyLevelDBType)

#endif
