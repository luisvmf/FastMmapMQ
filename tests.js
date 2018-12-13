var mmap=require("./fastmmapmq");
var id=mmap.CreateMmapSync("map1","rwx------");
var idh=mmap.CreateMmapSync("test","rwx------");
var idb=mmap.ConnectMmapSync("node","map1");
console.log(mmap.ListSync("node"));
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));

mmap.WriteAsync(id,"aaab");
mmap.WriteAsync(id,"aaab");
mmap.WriteSync(id,"aaab");
console.log(mmap.ReadSync(idb,0));

mmap.WriteSync(id,"aaac");
mmap.ReadAsync(idb,0,function(data){
	console.log(data);
});
mmap.CreateMmapAsync("map2","rwx------",function(idc){
	mmap.ConnectMmapAsync("node","map2",function(idd){
		mmap.WriteSync(idc,"test");
		console.log(mmap.ReadSync(idd,0));
	});
});
mmap.ListAsync("node",function(data){
	console.log(data);
});
mmap.WriteSharedStringSync(id,"String Sync");
console.log(mmap.GetSharedStringSync(idb));
console.log(mmap.GetSharedStringSync(idb));
console.log(mmap.GetSharedStringSync(idb));
