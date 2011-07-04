#include "leveldb_ext.h"

PyObject* leveldb_exception = 0;

static PyMethodDef leveldb_methods[] =
{
	LEVELDB_FUNC_DEF("RepairDB", leveldb_repair_db),
	{NULL, NULL},
};

#define LEVELDB_FUNC_DEF(pyfunc, func) { pyfunc, func, METH_VARARGS, func##_doc}

// helpers
#define TRY_MODULE_INIT(module, description) PyObject* module##_module = 0; if ((module##_module = Py_InitModule4(#module, module##_methods, description, 0, PYTHON_API_VERSION)) == 0) { return; }
#define TRY_MODULE_ADD_OBJECT(module, key, value) if (PyModule_AddObject(module, key, value) != 0) return;

PyMODINIT_FUNC
initleveldb(void)
{
	TRY_MODULE_INIT(leveldb, "LevelDB Python API");

	if (PyType_Ready(&PyLevelDBType) < 0)
		return;

	// add custom types to the different modules
	Py_INCREF(&PyLevelDBType);
	TRY_MODULE_ADD_OBJECT(leveldb_module, "LevelDB",               (PyObject*)&PyLevelDBType);

	// add custom exceptions
	leveldb_exception = PyErr_NewException("lebeldb.LevelDBError", 0, 0);

	if (leveldb_exception == 0)
		return;

	Py_INCREF(leveldb_exception);
	TRY_MODULE_ADD_OBJECT(leveldb_module, "LevelDBError", leveldb_exception);
}
