//Copyright (c) 2022 Luís Victor Muller Fabris. Apache License.
#include <nan.h>
#include <string>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include "cmodule.c"

using namespace v8;
using v8::Function;
using v8::Local;
using v8::Number;
using v8::Value;
using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::HandleScope;
using Nan::New;
using Nan::Null;
using Nan::To;
using v8::FunctionTemplate;
using namespace std::chrono;


class AsyncreadWorker : public AsyncWorker {
	public:
		 AsyncreadWorker(Callback *callback, int va,int vb)
			:AsyncWorker(callback),valinputnodereada(va),valinputnodereadb(vb) {}
		 ~AsyncreadWorker() {}
		  void Execute () {
				int a=valinputnodereada;  //id
				int b=valinputnodereadb;  //command
				nodereadretval=readmmap(a,b);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::String> jsArr = Nan::New(nodereadretval).ToLocalChecked();
			free(nodereadretval);
			int j=0;
			Local<Value> argv[] = {
				 jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int valinputnodereada;
		int valinputnodereadb;
		char *nodereadretval;
};
class AsyncConnectWorker : public AsyncWorker {
	public:
		 AsyncConnectWorker(Callback *callback, std::string internalstring, int length, std::string internalstringb, int lengthb)
			:AsyncWorker(callback),nodeinternalstring(internalstring),nodelength(length),nodeinternalstringb(internalstringb),nodelengthb(lengthb) {}
		 ~AsyncConnectWorker() {}
		  void Execute () {
				const char *str=nodeinternalstring.c_str(); //Program
				const char *strb=nodeinternalstringb.c_str(); //id
				nodeconretval=connectmmap(str,strb);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Integer> jsArr = Nan::New(nodeconretval);
			Local<Value> argv[] = {
				 jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int nodeconretval=0;
		std::string nodeinternalstring="";
		int nodelength;
		std::string nodeinternalstringb="";
		int nodelengthb;
};
class AsyncCreateWorker : public AsyncWorker {
	public:
		 AsyncCreateWorker(Callback *callback, std::string internalstring, int length, std::string internalstringb, int lengthb)
			:AsyncWorker(callback),nodeinternalstring(internalstring),nodelength(length),nodeinternalstringb(internalstringb),nodelengthb(lengthb) {}
		 ~AsyncCreateWorker() {}
		  void Execute () {
				const char *str=nodeinternalstring.c_str(); //Program
				const char *strb=nodeinternalstringb.c_str(); //id
				nodeconretval=createmmap(str,strb);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Integer> jsArr = Nan::New(nodeconretval);
			Local<Value> argv[] = {
				 jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int nodeconretval=0;
		std::string nodeinternalstring="";
		int nodelength;
		std::string nodeinternalstringb="";
		int nodelengthb;
};
class AsyncSendWorker : public AsyncWorker {
	public:
		 AsyncSendWorker(Callback *callback, int va, std::string internalstringb, int lengthb)
			:AsyncWorker(callback),valinputnodesend(va),nodesentstring(internalstringb),nodelengthb(lengthb) {}
		 ~AsyncSendWorker() {}
		  void Execute () {
				int a=valinputnodesend; //id
				const char *str=nodesentstring.c_str();  //Data
				nodesendretval=writemmap(a,str);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Integer> jsArr = Nan::New(nodesendretval);
			Local<Value> argv[] = {
				jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int nodesendretval=0;
		int valinputnodesend;
		std::string nodesentstring="";
		int nodelengthb;
};
class AsyncwritestringWorker : public AsyncWorker {
	public:
		 AsyncwritestringWorker(Callback *callback, int va, std::string internalstringb, int lengthb)
			:AsyncWorker(callback),valinputnodesend(va),nodesentstring(internalstringb),nodelengthb(lengthb) {}
		 ~AsyncwritestringWorker() {}
		  void Execute () {
				int vanode=valinputnodesend;
				char *tmpstring="";
				tmpstring=nodesentstring.c_str();
				writesharedstring(vanode,tmpstring);
				nodesendretval=0;
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Integer> jsArr = Nan::New(nodesendretval);
			Local<Value> argv[] = {
				jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int nodesendretval=0;
		int valinputnodesend;
		std::string nodesentstring="";
		int nodelengthb;
};
class AsyncreadstringWorker : public AsyncWorker {
	public:
		 AsyncreadstringWorker(Callback *callback, int va)
			:AsyncWorker(callback),valinputnodereada(va) {}
		 ~AsyncreadstringWorker() {}
		  void Execute () {
			int vanode=valinputnodereada;
			nodereadretval=getsharedstring(vanode);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::String> jsArr = Nan::New(nodereadretval).ToLocalChecked();
			free(nodereadretval);
			int j=0;
			Local<Value> argv[] = {
				 jsArr
			};
			callback->Call(1, argv);
		  }
	private:
		int valinputnodereada;
		int valinputnodereadb;
		char *nodereadretval;
};
void consync(const FunctionCallbackInfo<Value>& info) {
	v8::String::Utf8Value param1(info[0]->ToString());
	std::string internalstring = std::string(*param1);  
	Nan::Utf8String intdatastr(info[0]);
	int length = intdatastr.length();
	v8::String::Utf8Value param2(info[1]->ToString());
	std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	char *prog;
	prog=malloc(strlen(internalstring.c_str())+1);
	sprintf(prog,"%s",internalstring.c_str());
	int nodeconretval=connectmmap(prog,internalstringb.c_str());
	info.GetReturnValue().Set(nodeconretval);
}
void createsync(const FunctionCallbackInfo<Value>& info) {
	v8::String::Utf8Value param1(info[0]->ToString());
	std::string internalstring = std::string(*param1);  
	Nan::Utf8String intdatastr(info[0]);
	int length = intdatastr.length();
	v8::String::Utf8Value param2(info[1]->ToString());
	std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	char *prog;
	prog=malloc(strlen(internalstring.c_str())+1);
	sprintf(prog,"%s",internalstring.c_str());
	char *perm=internalstringb.c_str();
	int nodeconretval=createmmap(prog,perm);
	info.GetReturnValue().Set(nodeconretval);
}
void writesync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringbnode = std::string(*param2);  
	info.GetReturnValue().Set(writemmap(vanode,internalstringbnode.c_str()));
}
void writestringsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringbnode = std::string(*param2);
	int readmapindexselect=vanode;
	char stra[sharedstringsize+5]="";
	char *tmpstring="";
	tmpstring=stra;
	tmpstring=internalstringbnode.c_str();
	int i=memmappedarraysize;
	writesharedstring(readmapindexselect,tmpstring);
	info.GetReturnValue().Set(0);
}
void getstringsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
	char *b=getsharedstring(vanode);
	info.GetReturnValue().Set(Nan::New(b).ToLocalChecked());
	free(b);
}
void readsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
	int vbnode=info[1]->NumberValue();
	char *readsyncretval=readmmap(vanode,vbnode);
	info.GetReturnValue().Set(Nan::New(readsyncretval).ToLocalChecked());
	free(readsyncretval);
}
NAN_METHOD(write) {
	int va=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncSendWorker(callback,va,internalstringb,lengthb));
}
NAN_METHOD(read) {
	int va=info[0]->NumberValue();
	int vb=info[1]->NumberValue();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncreadWorker(callback,va,vb));
}
NAN_METHOD(readstringasync) {
	int va=info[0]->NumberValue();
	Callback *callback = new Callback(info[1].As<Function>());
	AsyncQueueWorker(new AsyncreadstringWorker(callback,va));
}

NAN_METHOD(writestringasync) {
	int va=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncwritestringWorker(callback,va,internalstringb,lengthb));
}
NAN_METHOD(connectmmap) {
    v8::String::Utf8Value param1(info[0]->ToString());
    std::string internalstring = std::string(*param1);  
	Nan::Utf8String intdatastr(info[0]);
	int length = intdatastr.length();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncConnectWorker(callback,internalstring,length,internalstringb,lengthb));
}
NAN_METHOD(createmmap) {
    v8::String::Utf8Value param1(info[0]->ToString());
    std::string internalstring = std::string(*param1);  
	Nan::Utf8String intdatastr(info[0]);
	int length = intdatastr.length();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncCreateWorker(callback,internalstring,length,internalstringb,lengthb));
}
NAN_MODULE_INIT(InitAll) {
	Nan::Set(target, Nan::New("WriteAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(write)).ToLocalChecked());
	Nan::Set(target, Nan::New("ReadAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(read)).ToLocalChecked());
	Nan::Set(target, Nan::New("WriteSharedStringAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(writestringasync)).ToLocalChecked());
	Nan::Set(target, Nan::New("GetSharedStringAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(readstringasync)).ToLocalChecked());
	Nan::Set(target, Nan::New("ConnectMmapAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(connectmmap)).ToLocalChecked());
	Nan::Set(target, Nan::New("CreateMmapAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(createmmap)).ToLocalChecked());
	NODE_SET_METHOD(target, "ConnectMmapSync", consync);
	NODE_SET_METHOD(target, "CreateMmapSync", createsync);
	NODE_SET_METHOD(target, "WriteSync", writesync);
	NODE_SET_METHOD(target, "ReadSync", readsync);
	NODE_SET_METHOD(target, "GetSharedStringSync", getstringsync);
	NODE_SET_METHOD(target, "WriteSharedStringSync", writestringsync);
}
NODE_MODULE(NativeExtension, InitAll)
