//Copyright (c) 2022 Luís Victor Muller Fabris. Apache License.
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <dirent.h> 
#include <time.h>
#include <sys/time.h>
#include "fastmmapmq.c"


static PyObject* writemessage(PyObject* self,  PyObject *args) {
	int writemapindexselect=0;
	char *s;
	if (!PyArg_ParseTuple(args, "is", &writemapindexselect, &s)) {
		return NULL;
	}
   return Py_BuildValue("i", fastmmapmq_writemmap(writemapindexselect,s));
}
static PyObject* readmessage(PyObject* self,  PyObject *args) {
	int readmapindexselect=0;
	int modemmap=0;
	if (!PyArg_ParseTuple(args, "ii", &readmapindexselect, &modemmap)) {
		return NULL;
	}
	char *tmpretval=fastmmapmq_readmmap(readmapindexselect,modemmap);
	PyObject* tmpretvalb=Py_BuildValue("s", tmpretval);
	free(tmpretval);
	return tmpretvalb;

}
static PyObject* pyinitmmap(PyObject* self,  PyObject *args) {
	char *s;
	char *b;
	if (!PyArg_ParseTuple(args, "ss",&b,&s)) {
		return NULL;
	}
	return Py_BuildValue("i", fastmmapmq_connectmmap(b,s));
}
static PyObject* pygetsharedstring(PyObject* self,  PyObject *args) {
	int readmapindexselect=-1;
	if (!PyArg_ParseTuple(args, "i",&readmapindexselect)) {
		return NULL;
	}
	char *tmpretval=fastmmapmq_getsharedstring(readmapindexselect);
	PyObject* tmpretvalb=Py_BuildValue("s", tmpretval);
	free(tmpretval);
	return tmpretvalb;
}
static PyObject* pywritesharedstring(PyObject* self,  PyObject *args) {
	int readmapindexselect=-1;
	char *tmpstring="";
	if (!PyArg_ParseTuple(args, "is",&readmapindexselect,&tmpstring)) {
		return NULL;
	}
	return Py_BuildValue("i", fastmmapmq_writesharedstring(readmapindexselect,tmpstring));
}
static PyObject* pyinitmmap_create(PyObject* self,  PyObject *args) {
	char *s;
	char *perm;
	int lockingmode;
	if (!PyArg_ParseTuple(args, "ssi",&s,&perm,&lockingmode)) {
		return NULL;
	}
	return Py_BuildValue("i", fastmmapmq_createmmap(s,perm,lockingmode));
}
static char mmap_docs_write[] =
   "write(id,'data'): Write 'data' into id message queue. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_read[] =
"read(id,bool): Read data from id message queue. Pass 0 to erase data when read so that other processes using read(id,0) can't see it.\n \
Pass 1 to read but don't erase to other processes using read(id,0) or read(id,1), just erase for current process.\n \
When using read(id,0) first read can get data written before this process started. First read(id,0) reads all data written to id and not read yet.\
First read(id,1) only reads data written after this proccess has called connected. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_connect[] =
   "connectmmap('filepath','id'): Connects to write/read messages from this id created by process in filepath.\
The id can contain any characters valid in a file name (no / or null character).This function must be called before write() or read().Returns mmapid.\n";
static char mmap_docs_create[] =
   "createmmap('id','permissions','lockmechanism'): Creates a mmap with id 'id' and permissions, using locking mode lockmechanism=0 for futex+spinlock and lockmechanism=1 for flock. Futex lock is faster but a deadlock may occur if one of the processes is terminated while a writemmap/ readmmap call is on progress. If the programs doing the read/write are independent, it is recommended using the flock, unless performance is critical..\nExample of permission 'rwx------' or 'rwxrwxrwx'. If the user has no write and read permissions on the mmap the program will segfault.";
static char mmap_docs_get_shared[] =
   "getsharedstring(id): Get a shared string that is avaliable to all programs connected to this mmap. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_write_shared[] =
   "writesharedstring(id,string): Writes a shared string thet is avaliable to all programs conncted to this mmap. id should be the value returned by connectmmap() or createmmap().";
static PyMethodDef mmap_funcs[] = {
	{"write", (PyCFunction)writemessage, METH_VARARGS, mmap_docs_write},
	{"read", (PyCFunction)readmessage, METH_VARARGS, mmap_docs_read},
	{"connectmmap", (PyCFunction)pyinitmmap, METH_VARARGS, mmap_docs_connect},
	{"createmmap", (PyCFunction)pyinitmmap_create, METH_VARARGS, mmap_docs_create},
	{"getsharedstring", (PyCFunction)pygetsharedstring, METH_VARARGS, mmap_docs_get_shared},
	{"writesharedstring", (PyCFunction)pywritesharedstring, METH_VARARGS, mmap_docs_write_shared},
	{ NULL, NULL, 0, NULL}
};
void initfastmmapmq(void) {
	Py_InitModule3("fastmmapmq", mmap_funcs,"Fast Message queue module, witch can pass messages between different processes, for Python, nodejs and c.");
}
