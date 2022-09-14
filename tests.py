import fastmmapmq
#help(fastmmap)
mapa=fastmmapmq.createmmap("testshm","rwx------",0)
mapb=fastmmapmq.connectmmap("python","testshm")
while(True):
	a=fastmmapmq.write(mapa,"test 1")
	if(a==0):
		break
print(fastmmapmq.read(mapb,0))
fastmmapmq.write(mapb,"test 2")
fastmmapmq.write(mapa,"test 3")
print(fastmmapmq.read(mapb,0))
fastmmapmq.write(mapa,"test 3")
print(fastmmapmq.read(mapb,1))
fastmmapmq.write(mapa,"test 3")
print(fastmmapmq.read(mapb,0))
print(fastmmapmq.getsharedstring(mapa))
print(fastmmapmq.getsharedstring(mapb))
fastmmapmq.writesharedstring(mapb,"Test")
print(fastmmapmq.getsharedstring(mapa))
print(fastmmapmq.getsharedstring(mapb))
