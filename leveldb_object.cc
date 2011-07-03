#include "leveldb_ext.h"

static void PyLevelDB_dealloc(PyLevelDB* self)
{
	if (self->db) {
		Py_BEGIN_ALLOW_THREADS
		delete self->db;
		Py_END_ALLOW_THREADS
		self->db = 0;
	}

	Py_TYPE(self)->tp_free(self);
}

static PyObject* PyLevelDB_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyLevelDB* self = (PyLevelDB*)type->tp_alloc(type, 0);

	if (self) {
		self->db = 0;
	}

	return (PyObject*)self;
}

static PyObject* PyLevelDB_Put(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	PyObject* sync = Py_False;
	Py_buffer key, value;
	static char* kwargs[] = {"key", "value", "sync", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s*s*|O!", kwargs, &key, &value, &PyBool_Type, &sync))
		return 0;

	leveldb::Slice key_slice((const char*)key.buf, (size_t)key.len);
	leveldb::Slice value_slice((const char*)value.buf, (size_t)value.len);

	leveldb::WriteOptions options;
	options.sync = (sync == Py_True) ? true : false;

	leveldb::Status status;

	Py_BEGIN_ALLOW_THREADS
	status = self->db->Put(options, key_slice, value_slice);
	Py_END_ALLOW_THREADS

	PyBuffer_Release(&key);
	PyBuffer_Release(&value);

	if (!status.ok()) {
		// TBD: exception
		printf("put failure\n");
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef PyLevelDB_methods[] = {
	{"Put",    (PyCFunction)PyLevelDB_Put, METH_KEYWORDS, "add a key/value pair to database, with an optional synchronous disk write" },
	{NULL}
};

static int PyLevelDB_init(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	if (self->db) {
		Py_BEGIN_ALLOW_THREADS
		delete self->db;
		Py_END_ALLOW_THREADS
		self->db = 0;
	}

	const char* db_dir = 0;

	PyObject* create_if_missing = Py_True;
	static char* kwargs[] = {"filename", "create_if_missing", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!", kwargs, &db_dir, &PyBool_Type, &create_if_missing))
		return -1;

	leveldb::Options options;
	options.create_if_missing = (create_if_missing == Py_True) ? true : false;
	leveldb::Status status;

	Py_BEGIN_ALLOW_THREADS
	status = leveldb::DB::Open(options, db_dir, &self->db);
	Py_END_ALLOW_THREADS

	if (!status.ok()) {
		printf("error in open\n");
		// TBD set exception
		return -1;
	}

	return 0;
}

PyTypeObject PyLevelDBType = {
	PyObject_HEAD_INIT(NULL)
	0,                               /*ob_size*/
	"leveldb.LevelDB",         /*tp_name*/
	sizeof(PyLevelDB),             /*tp_basicsize*/
	0,                               /*tp_itemsize*/
	(destructor)PyLevelDB_dealloc, /*tp_dealloc*/
	0,                               /*tp_print*/
	0,                               /*tp_getattr*/
	0,                               /*tp_setattr*/
	0,                               /*tp_compare*/
	0,                               /*tp_repr*/
	0,                               /*tp_as_number*/
	0,                               /*tp_as_sequence*/
	0,                               /*tp_as_mapping*/
	0,                               /*tp_hash */
	0,                               /*tp_call*/
	0,                               /*tp_str*/
	0,                               /*tp_getattro*/
	0,                               /*tp_setattro*/
	0,                               /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,              /*tp_flags*/
	"PyLevelDB wrapper",           /*tp_doc */
	0,                               /*tp_traverse */
	0,                               /*tp_clear */
	0,                               /*tp_richcompare */
	0,                               /*tp_weaklistoffset */
	0,                               /*tp_iter */
	0,                               /*tp_iternext */
	PyLevelDB_methods,             /*tp_methods */
	0,                               /*tp_members */
	0,                               /*tp_getset */
	0,                               /*tp_base */
	0,                               /*tp_dict */
	0,                               /*tp_descr_get */
	0,                               /*tp_descr_set */
	0,                               /*tp_dictoffset */
	(initproc)PyLevelDB_init,      /*tp_init */
	0,                               /*tp_alloc */
	PyLevelDB_new,                 /*tp_new */
};
