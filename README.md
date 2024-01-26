# FastMmapMQ
![alt tag](https://img.shields.io/badge/build-pass-green.svg)

	git clone https://git.luisvmf.com/FastMmapMQ

Fast Message queue module, witch can pass messages between different processes, for Python, nodejs and c written in C using a memory mapped file in /dev/shm.
Currently this only works on Linux because of requirements on /dev/shm and procfs with /proc/{PID}/fd.



To see the example of passing messages between different processes run:

	make c-demo

and then in one terminal run:

	./receive_example

leave the program running and in the other terminal run:

	./send_example

*These programs should not be renamed because binary name is on connectmmap function in send_example.c.

Example:

![](demo.gif)




# Avaliable functions:

	write(id,'data'): Write 'data' into id message queue. id should be the value returned by connectmmap() or createmmap(). Returns 0 on success and -1 if the write failed because buffer is full, ex the program reading isn't reading fast enough.

	read(id,bool): Read data from id message queue. Pass 0 to erase data when read so that other processes using read(id,0) can't see it. Pass 1 to read but don't erase to other processes using read(id,0) or read(id,1), just erase for current process. When using read(id,0) first read can get data written before this process started. First read(id,0) reads all data written to id and not read yet. First read(id,1) only reads data written after this proccess has called connected. id should be the value returned by connectmmap() or createmmap().

	connectmmap('filepath','id'): Connects to write/read messages from this id created by process in filepath. The id can contain any characters valid in a file name (no / or null character).This function must be called before write() or read().Returns mmapid.

	createmmap('id','permissions'): Creates a mmap with id 'id' and permissions.\nExample of permission 'rwx------' or 'rwxrwxrwx'. If the user has no write and read permissions on the mmap the program will segfault.

	getsharedstring(id): Get a shared string that is avaliable to all programs connected to this mmap. id should be the value returned by connectmmap() or createmmap().

	writesharedstring(id,string): Writes a shared string thet is avaliable to all programs conncted to this mmap. id should be the value returned by connectmmap() or createmmap().



# To build Python and Nodejs modules run:

	make python-module

	make node-module

Then check tests.py, tests.js and test.c for API examples,these files are just for demonstrating the API so they pass messages on the same process.

python tests.py

	testshm,testshm,testshm,testshm
	test 1 
	test 2 test 3 
	test 3 
	test 3 test 3 


	Test
	Test



node tests.js

	map1,test,map1,map1,map1
	aaab 
	aaab 
	aaab aaab aaab 
	String Sync
	String Sync
	String Sync
	aaac 
	String Async
	String Async
	map1,test,map1,map1,map1,map2,map1,test,map1,map1,map1
	test 

make c-demo

./test

	testmmapidb,testmmapbe,testmmapidb,testmmapidb,testmmapidb
	aaab aaab 
	aaac 
	
	
Benchmarks (with python module):

	Testing fastmmapmq:::...writing 20000000 messages:
	Time:10.7677519321s, with 31575 busy writes, 20031575 total write calls.

	Now testing multiprocessing queue. Writing  2000000 messages (100x less because this is slow):
	Time to write 2000000:14.1851100922s
	So to write 20000000 messages it would take:1418.51229668s

	Now testing string concatenation (this can't pass messages to another process, just for comparison):
	Time:4.2168469429s

