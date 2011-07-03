#include "leveldb_ext.h"

static PyMethodDef leveldb_methods[] =
{
	LEVELDB_FUNC_DEF("hello_world",       leveldb_hello_world),
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
}

const char leveldb_hello_world_doc[] =
"leveldb.HelloWorld()\n"
;
PyObject* leveldb_hello_world(PyObject* self, PyObject* args)
{
	Py_INCREF(Py_None);
	return Py_None;
}
