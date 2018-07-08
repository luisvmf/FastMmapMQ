import fastmmap
mapa=fastmmap.connectmmap("None","testshm")
mapb=fastmmap.connectmmap("python","testshm")
print fastmmap.listmaps("python")
fastmmap.write(mapa,"test 1")
print fastmmap.read(mapb,0)
fastmmap.write(mapa,"test 2")
fastmmap.write(mapa,"test 3")
print fastmmap.read(mapb,0)
