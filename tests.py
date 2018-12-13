import fastmmap
#help(fastmmap)
mapa=fastmmap.createmmap("testshm","rwx------")
mapb=fastmmap.connectmmap("python","testshm")
print fastmmap.listmaps("python")
while(True):
	a=fastmmap.write(mapa,"test 1")
	if(a==0):
		break
print fastmmap.read(mapb,0)
fastmmap.write(mapb,"test 2")
fastmmap.write(mapa,"test 3")
print fastmmap.read(mapb,0)
print fastmmap.getsharedstring(mapa)
print fastmmap.getsharedstring(mapb)
fastmmap.writesharedstring(mapb,"Test")
print fastmmap.getsharedstring(mapa)
print fastmmap.getsharedstring(mapb)
