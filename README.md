# FastMmapMQ
Fast Message queue module for Python, nodejs and c written in C using a memory mapped file in /dev/shm.
Check tests.py, tests.js and test.c for API.

./build-python.sh

./build-node.sh

./build-c.sh

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

	
./test

	testmmapidb,testmmapbe,testmmapidb,testmmapidb,testmmapidb
	aaab aaab 
	aaac 
	
	
benchmarks

	Testing fastmmapmq:::...writing 20000000 messages:
	Time:10.7677519321s, with 31575 busy writes, 20031575 total write calls.

	Now testing multiprocessing queue. Writing  2000000 messages (100x less because this is slow):
	Time to write 2000000:14.1851100922s
	So to write 20000000 messages it would take:1418.51229668s

	Now testing string concatenation (this can't pass messages to another process, just for comparison):
	Time:4.2168469429s

