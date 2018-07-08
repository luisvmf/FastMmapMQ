import fastmmap
import time
import ctypes
from multiprocessing import Process, Value, Queue
def procdemo():
	mapb=fastmmap.connectmmap("python tests.py","testshm")
	a=""
	while(True):
		a=a+fastmmap.read(mapb,0)
		if(a.index("test 3")>0):
			print a
			quit()
mapa=fastmmap.connectmmap("None","testshm")
print fastmmap.listmaps("python")
print "Tests:"
fastmmap.write(mapa,"test 1")
pdemo=Process(target=procdemo, args=())
pdemo.start()
fastmmap.write(mapa,"test 2")
fastmmap.write(mapa,"test 3")
time.sleep(3)
pdemo.terminate()
time.sleep(3)
def proca():
	mapc=fastmmap.connectmmap("python tests.py","testshm")
	while True:
		null=fastmmap.read(mapc,0)
p1=Process(target=proca, args=())
p1.start()
print "Testing fastmmapmq:::...writing 20000000 messages:"
i=0
t1=time.time()
while(i<20000000):
	fastmmap.write(mapa,"test"+str(i))
	i=i+1
print("Time:"+str(time.time()-t1)+"s")
data=fastmmap.read(mapa,0)
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
