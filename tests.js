var mmap=require("./fastmmapmq");
var id=mmap.ConnectMmapSync("None","map1");
var idb=mmap.ConnectMmapSync("node","map1");
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
mmap.ConnectMmapAsync("None","map2",function(data){
	mmap.ConnectMmapAsync("node","map2",function(datab){
		mmap.WriteSync(data,"test");
		console.log(mmap.ReadSync(datab,0));
	});
});
