# FastMmapMQ
Fast Message queue module, witch can pass messages between different processes, for Python, nodejs and c written in C using a memory mapped file in /dev/shm.
Currently this only works on Linux because of requirements on /dev/shm and procfs with /proc/{PID}/fd.



To see the working between different processes run:

	gcc -o send_example send_example.c
	gcc -o receive_example receive_example.c

and then on one terminal run:

	./receive_example

leave the program running and on the other terminal run:

	./send_example

*These programs should be run with this names, and should not be renamed because binary name is on connectmmap function in send_example.c.

![](demo.gif)

To build Python and Nodejs modules run:

	./build-python.sh

	./build-node.sh

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

./build-c.sh
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

