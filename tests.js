var mmap=require("./fastmmapmq");
var id=mmap.CreateMmapSync("map1","rwx------",0);
var idh=mmap.CreateMmapSync("test","rwx------",0);
var idb=mmap.ConnectMmapSync("node","map1");
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));

mmap.WriteAsync(id,"aaab",function(data){});
mmap.WriteAsync(id,"aaab",function(data){});
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));

mmap.WriteSync(id,"aaac");
mmap.ReadAsync(idb,0,function(data){
	console.log(data);
});
mmap.CreateMmapAsync("map2","rwx------",0,function(idc){
	mmap.ConnectMmapAsync("node","map2",function(idd){
		mmap.WriteSync(idc,"test");
		console.log(mmap.ReadSync(idd,0));
	});
});


mmap.WriteSharedStringSync(id,"String Sync");
console.log(mmap.GetSharedStringSync(idb));
console.log(mmap.GetSharedStringSync(idb));
console.log(mmap.GetSharedStringSync(idb));
mmap.WriteSharedStringAsync(id,"String Async",function(data){
	console.log(mmap.GetSharedStringSync(idb));
	mmap.GetSharedStringAsync(idb,function(data){
		console.log(data);
	});
});
