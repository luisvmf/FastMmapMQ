# FastMmapMQ
Fast Message queue Python module written in C using a memory mapped file in /dev/shm

./build-python.sh

./build-node.sh

./build-c.sh

python tests.py

	Tests:
	
	test 1 test 2 test 3 
	
	None
	
	Testing fastmmapmq:::...writing 20000000 messages:
	
	Time:8.45387196541s
	
	Now testing multiprocessing queue. Writing  2000000 messages (100x less because this is slow):
	
	Time to write 2000000:14.1614289284s
	
	So to write 20000000 messages it would take:1416.14449024s
	
	Now testing string concatenation (this can't pass messages to another process, just for comparison):
	
	Time:5.0640130043s


./node-v4.4.2 tests.js

	aaab 
	aaab 
	aaab aaab aaab 
	aaac 
	test 
	
./test

	aaab aaab

