import fastmmap
import time
import ctypes
from multiprocessing import Process, Value, Queue
def procdemo():
	fastmmap.connectmap("testshm")
	a=""
	while(True):
		a=a+fastmmap.read()
		if(a.index("test 3")>0):
			print a
			quit()
fastmmap.connectmap("testshm")
print "Tests:"
fastmmap.write("test 1")
pdemo=Process(target=procdemo, args=())
pdemo.start()
fastmmap.write("test 2")
fastmmap.write("test 3")
time.sleep(3)
pdemo.terminate()
time.sleep(3)
def proca():
	fastmmap.connectmap("testshm")
	while True:
		null=fastmmap.read()
p1=Process(target=proca, args=())
p1.start()
print "Testing fastmmapmq:::...writing 20000000 messages:"
i=0
t1=time.time()
while(i<20000000):
	fastmmap.write("test"+str(i))
	i=i+1
print("Time:"+str(time.time()-t1)+"s")
data=fastmmap.read()
p1.terminate()
time.sleep(3)
def procb(a,b):
	while(True):
		null=b.get()
managervar=Value(ctypes.c_char_p,'')
queuetest=Queue()
p2=Process(target=procb, args=(managervar,queuetest,))
p2.start()
print "Now testing multiprocessing queue. Writing  2000000 messages (100x less because this is slow):"
i=0
t1=time.time()
while(i<2000000):
	queuetest.put("test"+str(i))
	i=i+1
print "Time to write 2000000:"+str(time.time()-t1)+"s\nSo to write 20000000 messages it would take:"+str((time.time()-t1)*100)+"s"
p2.terminate()
print "Now testing string concatenation (this can't pass messages to another process, just for comparison):"
i=0
t1=time.time()
data=""
while(i<20000000):
	data="test"+str(i)
	i=i+1
print("Time:"+str((time.time()-t1))+"s")
quit()
