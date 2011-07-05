#include "leveldb_ext.h"

static void PyLevelDB_set_error(leveldb::Status& status)
{
	extern PyObject* leveldb_exception;
	PyErr_SetString(leveldb_exception, status.ToString().c_str());
}

const char leveldb_repair_db_doc[] =
"leveldb.RepairDB(db_dir)\n"
;
PyObject* leveldb_repair_db(PyObject* self, PyObject* args)
{
	const char* db_dir = 0;

	if (!PyArg_ParseTuple(args, "s", &db_dir))
		return 0;

	std::string _db_dir(db_dir);
	leveldb::Status status;
	leveldb::Options options;


	Py_BEGIN_ALLOW_THREADS
	status = leveldb::RepairDB(_db_dir.c_str(), options);
	Py_END_ALLOW_THREADS

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return 0;
	}


	Py_INCREF(Py_None);
	return Py_None;
}


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
		PyLevelDB_set_error(status);
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyLevelDB_Get(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	PyObject* verify_checksums = Py_False;
	PyObject* fill_cache = Py_True;
	Py_buffer key;
	static char* kwargs[] = {"key", "verify_checksums", "fill_cache", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s*|O!O!", kwargs, &key, &PyBool_Type, &verify_checksums, &PyBool_Type, &fill_cache))
		return 0;

	leveldb::Slice key_slice((const char*)key.buf, (size_t)key.len);

	leveldb::ReadOptions options;
	options.verify_checksums = (verify_checksums == Py_True) ? true : false;
	options.fill_cache = (fill_cache == Py_True) ? true : false;
	leveldb::Status status;
	std::string value;

	Py_BEGIN_ALLOW_THREADS
	status = self->db->Get(options, key_slice, &value);
	Py_END_ALLOW_THREADS

	PyBuffer_Release(&key);

	if (status.IsNotFound()) {
		PyErr_SetNone(PyExc_KeyError);
		return 0;
	}

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return 0;
	}

	return PyString_FromStringAndSize(value.c_str(), value.length());
}


static PyObject* PyLevelDB_Delete(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	PyObject* sync = Py_False;
	Py_buffer key;
	static char* kwargs[] = {"key", "sync", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s*|O!", kwargs, &key, &PyBool_Type, &sync))
		return 0;

	leveldb::Slice key_slice((const char*)key.buf, (size_t)key.len);

	leveldb::WriteOptions options;
	options.sync = (sync == Py_True) ? true : false;

	leveldb::Status status;

	Py_BEGIN_ALLOW_THREADS
	status = self->db->Delete(options, key_slice);
	Py_END_ALLOW_THREADS

	PyBuffer_Release(&key);

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
static PyMethodDef PyLevelDB_methods[] = {
	{"Put",    (PyCFunction)PyLevelDB_Put,       METH_KEYWORDS, "add a key/value pair to database, with an optional synchronous disk write" },
	{"Get",    (PyCFunction)PyLevelDB_Get,       METH_KEYWORDS, "get a value from the database" },
	{"Delete", (PyCFunction)PyLevelDB_Delete,    METH_KEYWORDS, "delete a value in the database" },
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
	PyObject* error_if_exists = Py_False;
	PyObject* paranoid_checks = Py_False;
	int write_buffer_size = 4<<20;
	int block_size = 4096;
	int max_open_files = 1000;
	int block_restart_interval = 16;
	static char* kwargs[] = {"filename", "create_if_missing", "error_if_exists", "paranoid_checks", "write_buffer_size", "block_size", "max_open_files", "block_restart_interval", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!O!O!iiii", kwargs,
		&db_dir,
		&PyBool_Type, &create_if_missing,
		&PyBool_Type, &error_if_exists,
		&PyBool_Type, &paranoid_checks,
		&write_buffer_size,
		&block_size,
		&max_open_files,
		&block_restart_interval))
		return -1;

	if (write_buffer_size < 0 || block_size < 0 || max_open_files < 0 || block_restart_interval < 0) {
		PyErr_SetString(PyExc_ValueError, "negative write_buffer_size/block_size/max_open_files/block_restart_interval");
		return -1;
	}

	leveldb::Options options;
	options.create_if_missing = (create_if_missing == Py_True) ? true : false;
	options.error_if_exists = (error_if_exists == Py_True) ? true : false;
	options.paranoid_checks = (paranoid_checks == Py_True) ? true : false;
	options.write_buffer_size = write_buffer_size;
	options.block_size = block_size;
	options.max_open_files = max_open_files;
	options.block_restart_interval = block_restart_interval;
	options.compression = leveldb::kSnappyCompression;
	leveldb::Status status;

	// copy string parameter, since we might lose when we release the GIL
	std::string _db_dir(db_dir);

	Py_BEGIN_ALLOW_THREADS
	status = leveldb::DB::Open(options, _db_dir, &self->db);
	Py_END_ALLOW_THREADS

	if (!status.ok()) {
		PyLevelDB_set_error(status);
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
