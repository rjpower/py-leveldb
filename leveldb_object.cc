#include "leveldb_ext.h"

static PyObject* LevelDBIter_new(PyLevelDB* db, leveldb::Iterator* iterator, std::string* to, int include_value);

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
	Py_BEGIN_ALLOW_THREADS
	delete self->_db;
	delete self->_options;
	delete self->_cache;
	Py_END_ALLOW_THREADS
	self->_db = 0;
	self->_options = 0;
	self->_cache = 0;
	Py_TYPE(self)->tp_free(self);
}

static void PyWriteBatch_dealloc(PyWriteBatch* self)
{
	delete self->ops;
	Py_TYPE(self)->tp_free(self);
}

static PyObject* PyLevelDB_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyLevelDB* self = (PyLevelDB*)type->tp_alloc(type, 0);

	if (self) {
		self->_db = 0;
		self->_options = 0;
		self->_cache = 0;
	}

	return (PyObject*)self;
}

static PyObject* PyWriteBatch_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyWriteBatch* self = (PyWriteBatch*)type->tp_alloc(type, 0);

	if (self) {
		self->ops = new std::vector<PyWriteBatchEntry>;

		if (self->ops == 0) {
			Py_TYPE(self)->tp_free(self);
			return PyErr_NoMemory();
		}
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
	status = self->_db->Put(options, key_slice, value_slice);
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
	status = self->_db->Get(options, key_slice, &value);
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
	status = self->_db->Delete(options, key_slice);
	Py_END_ALLOW_THREADS

	PyBuffer_Release(&key);

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyWriteBatch_Put(PyWriteBatch* self, PyObject* args)
{
	// NOTE: we copy all buffers
	Py_buffer key, value;

	if (!PyArg_ParseTuple(args, "s*s*", &key, &value))
		return 0;

	PyWriteBatchEntry op;
	op.is_put = true;
	op.key = std::string((const char*)key.buf, (size_t)key.len);
	op.value = std::string((const char*)value.buf, (size_t)value.len);

	PyBuffer_Release(&key);
	PyBuffer_Release(&value);

	self->ops->push_back(op);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyWriteBatch_Delete(PyWriteBatch* self, PyObject* args)
{
	// NOTE: we copy all buffers
	Py_buffer key;

	if (!PyArg_ParseTuple(args, "s*", &key))
		return 0;

	PyWriteBatchEntry op;
	op.is_put = false;
	op.key = std::string((const char*)key.buf, (size_t)key.len);

	PyBuffer_Release(&key);

	self->ops->push_back(op);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyLevelDB_Write(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	PyWriteBatch* write_batch = 0;
	PyObject* sync = Py_False;
	static char* kwargs[] = {"write_batch", "sync", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwargs, &PyWriteBatchType, &write_batch, &PyBool_Type, &sync))
		return 0;

	leveldb::WriteOptions options;
	options.sync = (sync == Py_True) ? true : false;

	leveldb::WriteBatch batch;
	leveldb::Status status;

	for (size_t i = 0; i < write_batch->ops->size(); i++) {
		PyWriteBatchEntry& op = (*write_batch->ops)[i];
		leveldb::Slice key(op.key.c_str(), op.key.size());
		leveldb::Slice value(op.value.c_str(), op.value.size());

		if (op.is_put) {
			batch.Put(key, value);
		} else {
			batch.Delete(key);
		}
	}

	Py_BEGIN_ALLOW_THREADS
	status = self->_db->Write(options, &batch);
	Py_END_ALLOW_THREADS

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* PyLevelDB_RangeIter(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	Py_buffer a, b;
	PyObject* verify_checksums = Py_False;
	PyObject* fill_cache = Py_True;
	PyObject* include_value = Py_True;
	static char* kwargs[] = {"key_from", "key_to", "verify_checksums", "fill_cache", "include_value", 0};

	a.buf = b.buf = 0;
	a.len = b.len = 0;
	a.obj = b.obj = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s*s*O!O!O!", kwargs, &a, &b, &PyBool_Type, &verify_checksums, &PyBool_Type, &fill_cache, &PyBool_Type, &include_value))
		return 0;

	leveldb::ReadOptions options;
	options.verify_checksums = (verify_checksums == Py_True) ? true : false;
	options.fill_cache = (fill_cache == Py_True) ? true : false;

	int is_from = (a.obj != 0);
	int is_to = (b.obj != 0);

	std::string from = std::string((const char*)a.buf, (size_t)a.len);
	std::string to = std::string((const char*)b.buf, (size_t)b.len);

	leveldb::Slice key(from.c_str(), from.size());

	if (a.obj)
		PyBuffer_Release(&a);

	if (b.obj)
		PyBuffer_Release(&b);

	// create iterator
	leveldb::Iterator* iter = 0;

	Py_BEGIN_ALLOW_THREADS
	iter = self->_db->NewIterator(options);
	Py_END_ALLOW_THREADS

	if (iter == 0)
		return PyErr_NoMemory();

	// position iterator
	Py_BEGIN_ALLOW_THREADS
	if (!is_from)
		iter->SeekToFirst();
	else
		iter->Seek(key);
	Py_END_ALLOW_THREADS

	// if iterator is empty, return an empty iterator object
	if (!iter->Valid()) {
		delete iter;
		return LevelDBIter_new(0, 0, 0, 0);
	}

	// otherwise, we're good
	std::string* s = 0;

	if (is_to) {
		s = new std::string(to);

		if (s == 0) {
			delete iter;
			return PyErr_NoMemory();
		}
	}

	return LevelDBIter_new(self, iter, s, (include_value == Py_True) ? 1 : 0);
}

static PyObject* PyLevelDB_GetStatus(PyLevelDB* self)
{
	std::string value;

	if (!self->_db->GetProperty(leveldb::Slice("leveldb.stats"), &value)) {
		PyErr_SetString(PyExc_ValueError, "unknown property");
		return 0;
	}

	return PyString_FromString(value.c_str());
}

static PyMethodDef PyLevelDB_methods[] = {
	{"Put",       (PyCFunction)PyLevelDB_Put,       METH_KEYWORDS, "add a key/value pair to database, with an optional synchronous disk write" },
	{"Get",       (PyCFunction)PyLevelDB_Get,       METH_KEYWORDS, "get a value from the database" },
	{"Delete",    (PyCFunction)PyLevelDB_Delete,    METH_KEYWORDS, "delete a value in the database" },
	{"Write",     (PyCFunction)PyLevelDB_Write,     METH_KEYWORDS, "apply a write-batch"},
	{"RangeIter", (PyCFunction)PyLevelDB_RangeIter, METH_KEYWORDS, "key/value range scan"},
	{"GetStats",  (PyCFunction)PyLevelDB_GetStatus, METH_NOARGS,   "get a mapping of all DB statistics"},
	{NULL}
};

static PyMethodDef PyWriteBatch_methods[] = {
	{"Put",    (PyCFunction)PyWriteBatch_Put,    METH_VARARGS, "add a put op to batch" },
	{"Delete", (PyCFunction)PyWriteBatch_Delete, METH_VARARGS, "add a delete op to batch" },
	{NULL}
};

static int PyLevelDB_init(PyLevelDB* self, PyObject* args, PyObject* kwds)
{
	// cleanup
	if (self->_db || self->_cache || self->_options) {
		Py_BEGIN_ALLOW_THREADS
		delete self->_db;
		delete self->_options;
		delete self->_cache;
		Py_END_ALLOW_THREADS
		self->_db = 0;
		self->_options = 0;
		self->_cache = 0;
	}

	// get params
	const char* db_dir = 0;

	PyObject* create_if_missing = Py_True;
	PyObject* error_if_exists = Py_False;
	PyObject* paranoid_checks = Py_False;
	int block_cache_size = 8 * (2 << 20);
	int write_buffer_size = 4<<20;
	int block_size = 4096;
	int max_open_files = 1000;
	int block_restart_interval = 16;
	static char* kwargs[] = {"filename", "create_if_missing", "error_if_exists", "paranoid_checks", "write_buffer_size", "block_size", "max_open_files", "block_restart_interval", "block_cache_size", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!O!O!iiiii", kwargs,
		&db_dir,
		&PyBool_Type, &create_if_missing,
		&PyBool_Type, &error_if_exists,
		&PyBool_Type, &paranoid_checks,
		&write_buffer_size,
		&block_size,
		&max_open_files,
		&block_restart_interval,
		&block_cache_size))
		return -1;

	if (write_buffer_size < 0 || block_size < 0 || max_open_files < 0 || block_restart_interval < 0 || block_cache_size < 0) {
		PyErr_SetString(PyExc_ValueError, "negative write_buffer_size/block_size/max_open_files/block_restart_interval/cache_size");
		return -1;
	}

	// open database
	self->_options = new leveldb::Options();
	self->_cache = leveldb::NewLRUCache(block_cache_size);

	if (self->_options == 0 || self->_cache == 0) {
		delete self->_options;
		delete self->_cache;
		self->_options = 0;
		self->_cache = 0;
		PyErr_NoMemory();
		return -1;
	}

	self->_options->create_if_missing = (create_if_missing == Py_True) ? true : false;
	self->_options->error_if_exists = (error_if_exists == Py_True) ? true : false;
	self->_options->paranoid_checks = (paranoid_checks == Py_True) ? true : false;
	self->_options->write_buffer_size = write_buffer_size;
	self->_options->block_size = block_size;
	self->_options->max_open_files = max_open_files;
	self->_options->block_restart_interval = block_restart_interval;
	self->_options->compression = leveldb::kSnappyCompression;
	self->_options->block_cache = self->_cache;
	leveldb::Status status;

	// note: copy string parameter, since we might lose when we release the GIL
	std::string _db_dir(db_dir);

	Py_BEGIN_ALLOW_THREADS
	status = leveldb::DB::Open(*self->_options, _db_dir, &self->_db);

	if (!status.ok()) {
		delete self->_db;
		delete self->_options;
		delete self->_cache;
		self->_options = 0;
		self->_cache = 0;
		self->_db = 0;
	}

	Py_END_ALLOW_THREADS

	if (!status.ok()) {
		PyLevelDB_set_error(status);
		return -1;
	}

	return 0;
}

static int PyWriteBatch_init(PyWriteBatch* self, PyObject* args, PyObject* kwds)
{
	self->ops->clear();
	static char* kwargs[] = {0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwargs))
		return -1;

	return 0;
}

PyTypeObject PyLevelDBType = {
	PyObject_HEAD_INIT(NULL)
	0,                             /*ob_size*/
	"leveldb.LevelDB",             /*tp_name*/
	sizeof(PyLevelDB),             /*tp_basicsize*/
	0,                             /*tp_itemsize*/
	(destructor)PyLevelDB_dealloc, /*tp_dealloc*/
	0,                             /*tp_print*/
	0,                             /*tp_getattr*/
	0,                             /*tp_setattr*/
	0,                             /*tp_compare*/
	0,                             /*tp_repr*/
	0,                             /*tp_as_number*/
	0,                             /*tp_as_sequence*/
	0,                             /*tp_as_mapping*/
	0,                             /*tp_hash */
	0,                             /*tp_call*/
	0,                             /*tp_str*/
	0,                             /*tp_getattro*/
	0,                             /*tp_setattro*/
	0,                             /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,            /*tp_flags*/
	"PyLevelDB wrapper",           /*tp_doc */
	0,                             /*tp_traverse */
	0,                             /*tp_clear */
	0,                             /*tp_richcompare */
	0,                             /*tp_weaklistoffset */
	0,                             /*tp_iter */
	0,                             /*tp_iternext */
	PyLevelDB_methods,             /*tp_methods */
	0,                             /*tp_members */
	0,                             /*tp_getset */
	0,                             /*tp_base */
	0,                             /*tp_dict */
	0,                             /*tp_descr_get */
	0,                             /*tp_descr_set */
	0,                             /*tp_dictoffset */
	(initproc)PyLevelDB_init,      /*tp_init */
	0,                             /*tp_alloc */
	PyLevelDB_new,                 /*tp_new */
};


PyTypeObject PyWriteBatchType = {
	PyObject_HEAD_INIT(NULL)
	0,                                /*ob_size*/
	"leveldb.WriteBatch",             /*tp_name*/
	sizeof(PyWriteBatch),             /*tp_basicsize*/
	0,                                /*tp_itemsize*/
	(destructor)PyWriteBatch_dealloc, /*tp_dealloc*/
	0,                                /*tp_print*/
	0,                                /*tp_getattr*/
	0,                                /*tp_setattr*/
	0,                                /*tp_compare*/
	0,                                /*tp_repr*/
	0,                                /*tp_as_number*/
	0,                                /*tp_as_sequence*/
	0,                                /*tp_as_mapping*/
	0,                                /*tp_hash */
	0,                                /*tp_call*/
	0,                                /*tp_str*/
	0,                                /*tp_getattro*/
	0,                                /*tp_setattro*/
	0,                                /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,               /*tp_flags*/
	"PyWriteBatch",                   /*tp_doc */
	0,                                /*tp_traverse */
	0,                                /*tp_clear */
	0,                                /*tp_richcompare */
	0,                                /*tp_weaklistoffset */
	0,                                /*tp_iter */
	0,                                /*tp_iternext */
	PyWriteBatch_methods,             /*tp_methods */
	0,                                /*tp_members */
	0,                                /*tp_getset */
	0,                                /*tp_base */
	0,                                /*tp_dict */
	0,                                /*tp_descr_get */
	0,                                /*tp_descr_set */
	0,                                /*tp_dictoffset */
	(initproc)PyWriteBatch_init,      /*tp_init */
	0,                                /*tp_alloc */
	PyWriteBatch_new,                 /*tp_new */
};

typedef struct {
    PyObject_HEAD
    PyLevelDB* _db;
	leveldb::Iterator* iterator;
	std::string* to;
	int include_value;
} LevelDBIter;

static void LevelDBIter_clean(LevelDBIter* iter)
{
    Py_XDECREF(iter->_db);
	delete iter->iterator;
	delete iter->to;
	iter->_db = 0;
	iter->iterator = 0;
	iter->to = 0;
	iter->include_value = 0;
}

static void LevelDBIter_dealloc(LevelDBIter* iter)
{
	LevelDBIter_clean(iter);
	PyObject_GC_Del(iter);
}

static int LevelDBIter_traverse(LevelDBIter* iter, visitproc visit, void* arg)
{
	Py_VISIT(iter->_db);
	return 0;
}

static PyObject* LevelDBIter_next(LevelDBIter* iter)
{
	// empty, do cleanup (idempotent)
	if (iter->_db == 0 || !iter->iterator->Valid()) {
		LevelDBIter_clean(iter);
		return 0;
	}

	// if we have an upper bound, and we have run past it, clean up and return
	if (iter->to) {
		leveldb::Slice a = leveldb::Slice(iter->to->c_str(), iter->to->size());
		leveldb::Slice b = iter->iterator->key();
		int c = iter->_db->_options->comparator->Compare(a, b);

		if (!(0 <= c)) {
			LevelDBIter_clean(iter);
			return 0;
		}
	}

	// get key and (optional) value
	PyObject* key = PyString_FromStringAndSize(iter->iterator->key().data(), iter->iterator->key().size());
	PyObject* value = 0;
	PyObject* ret = key;

	if (key == 0)
		return 0;

	if (iter->include_value) {
		value = PyString_FromStringAndSize(iter->iterator->value().data(), iter->iterator->value().size());

		if (value == 0) {
			Py_XDECREF(key);
			return 0;
		}
	}

	// key/value pairs are returned as 2-tuples
	if (value) {
		ret = PyTuple_New(2);

		if (ret == 0) {
			Py_DECREF(key);
			Py_XDECREF(value);
			return 0;
		}

		PyTuple_SET_ITEM(ret, 0, key);
		PyTuple_SET_ITEM(ret, 1, value);
	}

	// get next value
	iter->iterator->Next();

	// return k/v pair or single key
	return ret;
}

PyTypeObject PyLevelDBIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "leveldb-keyiterator",           /* tp_name */
    sizeof(LevelDBIter),             /* tp_basicsize */
    0,                               /* tp_itemsize */
    (destructor)LevelDBIter_dealloc, /* tp_dealloc */
    0,                               /* tp_print */
    0,                               /* tp_getattr */
    0,                               /* tp_setattr */
    0,                               /* tp_compare */
    0,                               /* tp_repr */
    0,                               /* tp_as_number */
    0,                               /* tp_as_sequence */
    0,                               /* tp_as_mapping */
    0,                               /* tp_hash */
    0,                               /* tp_call */
    0,                               /* tp_str */
    PyObject_GenericGetAttr,         /* tp_getattro */
    0,                               /* tp_setattro */
    0,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT  | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                               /* tp_doc */
    (traverseproc)LevelDBIter_traverse,  /* tp_traverse */
    0,                               /* tp_clear */
    0,                               /* tp_richcompare */
    0,                               /* tp_weaklistoffset */
    PyObject_SelfIter,               /* tp_iter */
    (iternextfunc)LevelDBIter_next,  /* tp_iternext */
    0,                               /* tp_methods */
    0,
};

static PyObject* LevelDBIter_new(PyLevelDB* db, leveldb::Iterator* iterator, std::string* to, int include_value)
{
	LevelDBIter* iter = PyObject_GC_New(LevelDBIter, &PyLevelDBIter_Type);

	if (iter == 0)
		return 0;

	Py_XINCREF(db);
	iter->_db = db;
	iter->iterator = iterator;
	iter->to = to;
	iter->include_value = include_value;
	_PyObject_GC_TRACK(iter);
	return (PyObject*)iter;
}
