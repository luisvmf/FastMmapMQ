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
//Copyright (c) 2018 Luís Victor Muller Fabris. Apache License.
//TODO:Add buffer overrun warning.
//---------------------------------------------------------
//---------------------------------------------------------
//Constants
#define shmfolder "/dev/shm"
#define shmpath "/dev/shm/luisvmfcomfastmmapmqshm-"
#define bufferlength  (999999) //Number of characters in buffer. Maximum value is 999999.
#define maxmemreturnsize  (999999) //Maximum number of characters returned by read function in one run. Maximum value is 999999.
//---------------------------------------------------------
//---------------------------------------------------------
#define memmappedarraysize  (bufferlength+100)
#define shmsize (memmappedarraysize * sizeof(char))
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
int fd[65535];
volatile char *map[memmappedarraysize+100];
int currentcreatedmapindex=0;
int readpos;
int indexb=0;
int searchb(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	char temp[memmappedarraysize+35];
	if((fp=fopen(fname,"r"))==NULL){
		return 0;
	}
	while(fgets(temp, memmappedarraysize+35, fp)!=NULL){
		int isearch=0;
		while(isearch<memmappedarraysize+35){
			if(temp[isearch]=='\0'){
				temp[isearch]='\x17';
			}
			isearch=isearch+1;
		}
		temp[memmappedarraysize+34]='\0';
		if((strstr(temp, str))!=NULL){
			return 1;
			find_result++;
		}
		line_num++;
	}
	if(fp) {
		fclose(fp);
	}
   	return 0;
}
int search(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	int isearch=0;
	char temp[memmappedarraysize+35];
	while(isearch<memmappedarraysize+35){
		temp[isearch]='\0';
		isearch=isearch+1;
	}
	if((fp=fopen(fname,"r"))==NULL){
		return 0;
	}
	while(fgets(temp, memmappedarraysize+35, fp)!=NULL){
		isearch=0;
		while(isearch<memmappedarraysize+35){
			if(temp[isearch]=='\0'){
				if((temp[isearch+1]!='\0') || (temp[isearch+2]!='\0') || (temp[isearch+3]!='\0') || (temp[isearch+4]!='\0')){
					temp[isearch]=' ';
				}
			}
			isearch=isearch+1;
		}
		if((strstr(temp, str))!=NULL){
			return 1;
			find_result++;
		}
		line_num++;
	}
	if(fp) {
		fclose(fp);
	}
   	return 0;
}
int openfd(char *programlocation,char *id){
	srand(time(NULL));
	if(strcmp(programlocation,"None")!=0){
			int foundfile=0;
			DIR *d;
			struct dirent *dir;
			d=opendir("/proc/");
			if(d){
			while((dir=readdir(d))!=NULL){
			if(foundfile==1){
				break;
			}
			int tmpnum = atoi(dir->d_name);
			if(tmpnum==0&&(dir->d_name)[0]!='0'){}else{
				char *cmdlineuri;
				cmdlineuri=malloc(strlen((dir->d_name))+strlen("/proc//cmdline")+1);
				strcpy(cmdlineuri, "/proc/");
				strcat(cmdlineuri, (dir->d_name));
				strcat(cmdlineuri, "/cmdline");
				if(search(cmdlineuri,programlocation)){
				char *cmdlinefduri;
				cmdlinefduri=malloc(strlen((dir->d_name))+strlen("/proc//fd/")+1);
				strcpy(cmdlinefduri, "/proc/");
				strcat(cmdlinefduri, (dir->d_name));
				strcat(cmdlinefduri, "/fd/");
				DIR *db;
				struct dirent *dirb;
				db=opendir(cmdlinefduri);
				if(db){
				while((dirb=readdir(db))!=NULL){
				int tmpnumb = atoi(dirb->d_name);
				if(tmpnumb==0&&(dirb->d_name)[0]!='0'){}else{
						char *cmdlinefduric;
						cmdlinefduric=malloc(strlen((dir->d_name))+strlen("/proc//fd/")+1+5);
						strcpy(cmdlinefduric, "/proc/");
						strcat(cmdlinefduric, (dir->d_name));
						strcat(cmdlinefduric, "/fd/");
						strcat(cmdlinefduric, (dirb->d_name));
						struct stat sb;
						if(stat(cmdlinefduric, &sb)!=-1){
							if(S_ISREG(sb.st_mode)){
								if(searchb(cmdlinefduric,"luisvmffastmmapmq")){
									//char *cmdlinefduric is the file uri in /proc for an unlinked fastmmapmq adress owened by program with cmdline containing char *programlocation!
									//Lets check if this file contains char *id
									if(searchb(cmdlinefduric,id)){
										//We found the unlinked file we want!!! Now lets open an file descriptor to it!!!
										//printf("found pid:%s, program: %s, file descriptor:%s\n",(dir->d_name),cmdlinefduric,(dirb->d_name));
										foundfile=1;
										char* location;
										location=malloc(strlen(shmpath)+strlen(id)+1);
										strcpy(location, cmdlinefduric);
										fd[currentcreatedmapindex] = open(location, O_RDWR | O_CREAT, (mode_t)0600);
										if (fd[currentcreatedmapindex] == -1) {
											foundfile=0;
										}

									}
								}
							}
						}
				}
				if(foundfile==1){
					break;
				}
				}
				closedir(db);
				}
				}
			}
			}
			closedir(d);
			}
	}else{
		char strab[29]="";
		char *randomstring;
		randomstring=strab;
		sprintf(randomstring,"%i",(rand()));
		char* location;
		location=malloc(strlen(shmpath)+strlen(randomstring)+1);
		strcpy(location, shmpath);
		strcat(location, randomstring);
		fd[currentcreatedmapindex] = open(location, O_RDWR | O_CREAT, (mode_t)0600);
		if (fd[currentcreatedmapindex] == -1) {
			perror("Error opening shared memory");
			exit(EXIT_FAILURE);
		}
		char* locationbfgffsthf;
		locationbfgffsthf=malloc(strlen(shmpath)+strlen(randomstring)+1);
		strcpy(locationbfgffsthf, shmpath);
		strcat(locationbfgffsthf, randomstring);
		unlink(locationbfgffsthf);
		}
	currentcreatedmapindex=currentcreatedmapindex+1;
	return currentcreatedmapindex-1;
}
void initshm(){
	int lseekresult;
	lseekresult = lseek(fd[currentcreatedmapindex-1], shmsize-1, SEEK_SET);
	if (lseekresult == -1) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on lseek");
		exit(EXIT_FAILURE);
	}
	lseekresult = write(fd[currentcreatedmapindex-1], "", 1);
	if (lseekresult != 1) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on shared memory initialization");
		exit(EXIT_FAILURE);
	}
}
void creatememmap(){
	map[currentcreatedmapindex-1] = mmap(0, shmsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd[currentcreatedmapindex-1], 0);
	if (map[currentcreatedmapindex-1] == MAP_FAILED) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on shared memory mmap");
		exit(EXIT_FAILURE);
	}
}
int startmemmap(char *programlocation,char *id){
	int thismapindex=openfd(programlocation,id);
	initshm();
	creatememmap();
	char strab[9]="";
	char *dataposb;
	dataposb=strab;
	indexb=0;
	if(map[currentcreatedmapindex-1][7]!='\x17'){
		map[currentcreatedmapindex-1][0]='0';
		map[currentcreatedmapindex-1][1]='0';
		map[currentcreatedmapindex-1][2]='0';
		map[currentcreatedmapindex-1][3]='0';
		map[currentcreatedmapindex-1][4]='0';
		map[currentcreatedmapindex-1][5]='0';
		map[currentcreatedmapindex-1][6]='0';
		map[currentcreatedmapindex-1][8]='0';
		map[currentcreatedmapindex-1][9]='0';
		map[currentcreatedmapindex-1][10]='0';
		map[currentcreatedmapindex-1][11]='0';
		map[currentcreatedmapindex-1][12]='0';
		map[currentcreatedmapindex-1][13]='0';
		map[currentcreatedmapindex-1][14]='0';
		map[currentcreatedmapindex-1][15]='0';
		map[currentcreatedmapindex-1][16]='0';
	}
	map[currentcreatedmapindex-1][7]='\x17';
	dataposb[0]=map[currentcreatedmapindex-1][0];
	dataposb[1]=map[currentcreatedmapindex-1][1];
	dataposb[2]=map[currentcreatedmapindex-1][2];
	dataposb[3]=map[currentcreatedmapindex-1][3];
	dataposb[4]=map[currentcreatedmapindex-1][4];
	dataposb[5]=map[currentcreatedmapindex-1][5];
	dataposb[6]=map[currentcreatedmapindex-1][6];
	indexb=atoi(dataposb);//-17  (estava funcionando assim no python)???
	map[currentcreatedmapindex-1][shmsize-40]=id[0];
	map[currentcreatedmapindex-1][shmsize-39]=id[1];
	map[currentcreatedmapindex-1][shmsize-38]=id[2];
	map[currentcreatedmapindex-1][shmsize-37]=id[3];
	map[currentcreatedmapindex-1][shmsize-36]=id[4];
	map[currentcreatedmapindex-1][shmsize-35]=id[5];
	map[currentcreatedmapindex-1][shmsize-34]=id[6];
	map[currentcreatedmapindex-1][shmsize-33]=id[7];
	map[currentcreatedmapindex-1][shmsize-32]=id[8];
	map[currentcreatedmapindex-1][shmsize-31]=id[9];
	map[currentcreatedmapindex-1][shmsize-30]=id[10];
	map[currentcreatedmapindex-1][shmsize-29]=id[11];
	map[currentcreatedmapindex-1][shmsize-28]=id[12];
	map[currentcreatedmapindex-1][shmsize-27]=id[13];
	map[currentcreatedmapindex-1][shmsize-26]=id[14];
	map[currentcreatedmapindex-1][shmsize-25]=id[15];
	map[currentcreatedmapindex-1][shmsize-24]=id[16];
	map[currentcreatedmapindex-1][shmsize-23]=id[17];
	map[currentcreatedmapindex-1][shmsize-22]=id[18];
	map[currentcreatedmapindex-1][shmsize-21]=id[19];
	map[currentcreatedmapindex-1][shmsize-20]='l';
	map[currentcreatedmapindex-1][shmsize-19]='u';
	map[currentcreatedmapindex-1][shmsize-18]='i';
	map[currentcreatedmapindex-1][shmsize-17]='s';
	map[currentcreatedmapindex-1][shmsize-16]='v';
	map[currentcreatedmapindex-1][shmsize-15]='m';
	map[currentcreatedmapindex-1][shmsize-14]='f';
	map[currentcreatedmapindex-1][shmsize-13]='f';
	map[currentcreatedmapindex-1][shmsize-12]='a';
	map[currentcreatedmapindex-1][shmsize-11]='s';
	map[currentcreatedmapindex-1][shmsize-10]='t';
	map[currentcreatedmapindex-1][shmsize-9]='m';
	map[currentcreatedmapindex-1][shmsize-8]='m';
	map[currentcreatedmapindex-1][shmsize-7]='a';
	map[currentcreatedmapindex-1][shmsize-6]='p';
	map[currentcreatedmapindex-1][shmsize-5]='m';
	map[currentcreatedmapindex-1][shmsize-4]='q';
	map[currentcreatedmapindex-1][shmsize-3]='\x17';
	map[currentcreatedmapindex-1][shmsize-2]='\x17';
	map[currentcreatedmapindex-1][shmsize-1]='\x17';
	return thismapindex;
}
void addresetcounter(int thismapindexreset){
	char stra[9]="";
	char *dataposbc;
	dataposbc=stra;
	int indexc=0;
	sprintf(dataposbc,"%c%c",map[thismapindexreset][15],map[thismapindexreset][16]);
	indexc=atoi(dataposbc);
	indexc=indexc+1;
	int vindexca=indexc/10;
	map[thismapindexreset][15]=vindexca+'0';
	map[thismapindexreset][16]=(indexc-vindexca*10)+'0';
}
char *readutils(int readmapindexselect,int gfifghdughfid){
	if(gfifghdughfid==0){	
		//The loop below is sometimes optimized by the compiler and the program crashes???
		//This is to avoid race between multiple threads.
		while(map[readmapindexselect][shmsize-41]=='A'){
		}
		char stra[maxmemreturnsize+100]="";
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		char *datapos;
		datapos=strab;
		int index=0;
		datapos[0]=map[readmapindexselect][0];
		datapos[1]=map[readmapindexselect][1];
		datapos[2]=map[readmapindexselect][2];
		datapos[3]=map[readmapindexselect][3];
		datapos[4]=map[readmapindexselect][4];
		datapos[5]=map[readmapindexselect][5];
		datapos[6]=map[readmapindexselect][6];
		index=atoi(datapos);
		char *dataposb;
		dataposb=strab;
		indexb=0;
		dataposb[0]=map[readmapindexselect][8];
		dataposb[1]=map[readmapindexselect][9];
		dataposb[2]=map[readmapindexselect][10];
		dataposb[3]=map[readmapindexselect][11];
		dataposb[4]=map[readmapindexselect][12];
		dataposb[5]=map[readmapindexselect][13];
		dataposb[6]=map[readmapindexselect][14];
		indexb=atoi(dataposb);
		char *dataposbc;
		dataposbc=strab;
		int indexc=0;
		sprintf(dataposbc,"%c%c",map[readmapindexselect][15],map[readmapindexselect][16]);
		indexc=atoi(dataposbc);
		int i=indexb+17;
		if(indexb==index-17){
			return "";
		}else{
		if(indexb==index){
		if(index==0){
			return "";
		}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<bufferlength+17){
				tmpstr[i-indexb-17]=map[readmapindexselect][i];
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[i-indexb-17]=' ';
				}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-indexb-17;
			indexb=0;
			i=indexb+17;
			while(i<index){
				tmpstr[offsetretarray+i-indexb-17]=map[readmapindexselect][i];
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-indexb-17]=' ';
				}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				tmpstr[i-indexb-17]=map[readmapindexselect][i];
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}
		map[readmapindexselect][shmsize-41]='A';
		indexb=indexb+i-indexb-17;
		int vca=indexb/1000000;
		int vcb=(indexb-vca*1000000)/100000;
		int vcc=(indexb-vca*1000000-vcb*100000)/10000;
		int vcd=(indexb-vca*1000000-vcb*100000-vcc*10000)/1000;
		int vce=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000)/100;
		int vcf=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100)/10;
		int vcg=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100-vcf*10);
		map[readmapindexselect][8]=vca+'0';
		map[readmapindexselect][9]=vcb+'0';
		map[readmapindexselect][10]=vcc+'0';
		map[readmapindexselect][11]=vcd+'0';
		map[readmapindexselect][12]=vce+'0';
		map[readmapindexselect][13]=vcf+'0';
		map[readmapindexselect][14]=vcg+'0';
		map[readmapindexselect][shmsize-41]='\0';
		return tmpstr;
		}
	}else{
		char stra[maxmemreturnsize+100]="";
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		char *datapos;
		datapos=strab;
		int index=0;
		datapos[0]=map[readmapindexselect][0];
		datapos[1]=map[readmapindexselect][1];
		datapos[2]=map[readmapindexselect][2];
		datapos[3]=map[readmapindexselect][3];
		datapos[4]=map[readmapindexselect][4];
		datapos[5]=map[readmapindexselect][5];
		datapos[6]=map[readmapindexselect][6];
		index=atoi(datapos);
		char *dataposb;
		dataposb=strab;
		char *dataposbc;
		dataposbc=strab;
		int indexc=0;
		sprintf(dataposbc,"%c%c",map[readmapindexselect][15],map[readmapindexselect][16]);
		indexc=atoi(dataposbc);
		int i=indexb+17;
		if(indexb==index-17){
			return "";
		}else{
		if(indexb==index){
		if(index==0){
			return "";
		}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<bufferlength+17){
				tmpstr[i-indexb-17]=map[readmapindexselect][i];
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[i-indexb-17]=' ';
				}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-indexb-17;
			indexb=0;
			i=indexb+17;
			while(i<index){
				tmpstr[offsetretarray+i-indexb-17]=map[readmapindexselect][i];
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-indexb-17]=' ';
				}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				tmpstr[i-indexb-17]=map[readmapindexselect][i];
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}
		indexb=indexb+i-indexb-17;
		return tmpstr;
		}
	}
}
int writeutils(int writemapindexselect,char *s){
	char stra[9]="";
	char *datapos;
	datapos=stra;
	//The loop below is sometimes optimized by the compiler and the program crashes???
	//This is to avoid race between multiple threads.
	while(map[writemapindexselect][shmsize-42]=='A'){
	}
	map[writemapindexselect][shmsize-42]='A';
	datapos[0]=map[writemapindexselect][0];
	datapos[1]=map[writemapindexselect][1];
	datapos[2]=map[writemapindexselect][2];
	datapos[3]=map[writemapindexselect][3];
	datapos[4]=map[writemapindexselect][4];
	datapos[5]=map[writemapindexselect][5];
	datapos[6]=map[writemapindexselect][6];
	int index=0;
	int i=0;
	index=atoi(datapos);
	index=index+1;
	if(index==1){
		index=18;
	}
	i=index;
	int lenscalc=strlen(s);
	int resetwriteposf=0;
	int oldiwrite=0;
	while(i<=lenscalc+index){
		map[writemapindexselect][i-1]=s[i-index];
		i=i+1;
		oldiwrite=i;
		if(i>bufferlength+17){
			resetwriteposf=1;
			oldiwrite=i;
			i=lenscalc+index+1;
			break;
		}
	}
	i=oldiwrite;
	if(resetwriteposf==1){
		addresetcounter(writemapindexselect);
		i=18;
		while(i<=lenscalc+18-(oldiwrite-index)){
			map[writemapindexselect][i-1]=s[oldiwrite+i-index-18];
			i=i+1;
		}
	}
	map[writemapindexselect][i-2]=' ';
	index=i-1;
	int vca=index/1000000;
	int aux=index-vca*1000000;
	int vcb=(aux)/100000;
	aux=aux-vcb*100000;
	int vcc=(aux)/10000;
	aux=aux-vcc*10000;
	int vcd=(aux)/1000;
	aux=aux-vcd*1000;
	int vce=(aux)/100;
	aux=aux-vce*100;
	int vcf=(aux)/10;
	aux=aux-vcf*10;
	int vcg=(aux);
	map[writemapindexselect][0]=vca+'0';
	map[writemapindexselect][1]=vcb+'0';
	map[writemapindexselect][2]=vcc+'0';
	map[writemapindexselect][3]=vcd+'0';
	map[writemapindexselect][4]=vce+'0';
	map[writemapindexselect][5]=vcf+'0';
	map[writemapindexselect][6]=vcg+'0';
	if(index>=bufferlength+18){
		map[writemapindexselect][0]='0';
		map[writemapindexselect][1]='0';
		map[writemapindexselect][2]='0';
		map[writemapindexselect][3]='0';
		map[writemapindexselect][4]='0';
		map[writemapindexselect][5]='0';
		map[writemapindexselect][6]='0';
		char *dataposbc;
		dataposbc=stra;
		int indexc=0;
		sprintf(dataposbc,"%c%c",map[writemapindexselect][15],map[writemapindexselect][16]);
		indexc=atoi(dataposbc);
		indexc=indexc+1;
		int vindexca=indexc/10;
		map[writemapindexselect][15]=vindexca+'0';
		map[writemapindexselect][16]=(indexc-vindexca*10)+'0';
	}
	map[writemapindexselect][shmsize-42]='\0';
   return 0;
}
class AsyncreadWorker : public AsyncWorker {
	public:
		 AsyncreadWorker(Callback *callback, int va,int vb)
			:AsyncWorker(callback),valinputnodereada(va),valinputnodereadb(vb) {}
		 ~AsyncreadWorker() {}
		  void Execute () {
				int a=valinputnodereada;  //id
				int b=valinputnodereadb;  //command
				nodereadretval=readutils(a,b);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::String> jsArr = Nan::New(nodereadretval).ToLocalChecked();
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
				char *prog;
				prog=malloc(strlen(str)+1);
				sprintf(prog,"%s",str);
				nodeconretval=startmemmap(prog,strb);
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
				nodesendretval=writeutils(a,str);
		  }
		  void HandleOKCallback () {
			Nan::HandleScope scope;
			v8::Local<v8::Integer> jsArr = Nan::New(nodesendretval);
			Local<Value> argv[] = {
				jsArr
			};
		  }
	private:
		int nodesendretval=0;
		int valinputnodesend;
		std::string nodesentstring="";
		int nodelengthb;
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
	int nodeconretval=startmemmap(prog,internalstringb.c_str());
	info.GetReturnValue().Set(nodeconretval);
}
void writesync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringbnode = std::string(*param2);  
	info.GetReturnValue().Set(writeutils(vanode,internalstringbnode.c_str()));
}
void readsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
	int vbnode=info[1]->NumberValue();
	char *readsyncretval=readutils(vanode,vbnode);
	info.GetReturnValue().Set(Nan::New(readsyncretval).ToLocalChecked());
}
NAN_METHOD(write) {
	int va=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringb = std::string(*param2);  
	Nan::Utf8String intdatastrb(info[1]);
	int lengthb = intdatastrb.length();
	Callback *callback = new Callback();
	AsyncQueueWorker(new AsyncSendWorker(callback,va,internalstringb,lengthb));
}
NAN_METHOD(read) {
	int va=info[0]->NumberValue();
	int vb=info[1]->NumberValue();
	Callback *callback = new Callback(info[2].As<Function>());
	AsyncQueueWorker(new AsyncreadWorker(callback,va,vb));
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
#ifndef ASYNC_TEST_H_
#define ASYNC_TEST_H_
NAN_METHOD(write);
NAN_METHOD(connectmmap);
#endif
NAN_MODULE_INIT(InitAll) {
	Nan::Set(target, Nan::New("WriteAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(write)).ToLocalChecked());
	Nan::Set(target, Nan::New("ReadAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(read)).ToLocalChecked());
	Nan::Set(target, Nan::New("ConnectMmapAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(connectmmap)).ToLocalChecked());
	NODE_SET_METHOD(target, "ConnectMmapSync", consync);
	NODE_SET_METHOD(target, "WriteSync", writesync);
	NODE_SET_METHOD(target, "ReadSync", readsync);
}
NODE_MODULE(NativeExtension, InitAll)
