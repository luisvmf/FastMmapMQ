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
//TODO: Implement a queue in WriteSync and WriteAsync in case of temporary fail instead of just returning -1.
//---------------------------------------------------------
//---------------------------------------------------------
//Constants. This constants should be the same for all programs using a mmap. Diferent constants for the same mmap can result in undefined behavior.
#define shmfolder "/dev/shm"
#define shmpath "/dev/shm/luisvmfcomfastmmapmqshm-"
#define bufferlength  (999999) //Number of characters in buffer. Maximum value is 999999.
#define maxmemreturnsize  (999999) //Maximum number of characters returned by read function in one run. Maximum value is 999999.
#define sharedstringsize  (999999) //Maximum number of characters in shared string. No maximum.
//---------------------------------------------------------
//---------------------------------------------------------
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
int fd[bufferlength];
volatile char *map[bufferlength+200];
int currentcreatedmapindex=0;
int indexb[bufferlength]={0};
int searchb(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	char temp[45+35];
	if((fp=fopen(fname,"r"))==NULL){
		return 0;
	}
	lseek((intptr_t)fp, shmsize-45, SEEK_SET);
	while(fgets(temp,45, fp)!=NULL){
		int isearch=0;
		while(isearch<45){
			if(temp[isearch]=='\0'){
				temp[isearch]='\x17';
			}
			isearch=isearch+1;
		}
		temp[45+34]='\0';
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
	char temp[350];
	while(isearch<350){
		temp[isearch]='\0';
		isearch=isearch+1;
	}
	if((fp=fopen(fname,"r"))==NULL){
		return 0;
	}
	while(fgets(temp, 350, fp)!=NULL){
		isearch=0;
		while(isearch<350){
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
int isinshm(char *fdlink){
	struct stat sb;
	char *linkname;
	ssize_t r;
	char *str="/dev/shm";
	if (lstat(fdlink, &sb) == -1) {
		perror("lstat");
		exit(EXIT_FAILURE);
	}
	linkname = malloc(sb.st_size + 1);
	if (linkname == NULL) {
		fprintf(stderr, "Fail in isinshm\n");
		exit(EXIT_FAILURE);
	}
	r = readlink(fdlink, linkname, sb.st_size + 1);
	if (r < 0) {
		perror("lstat");
		exit(EXIT_FAILURE);
	}
	if (r > sb.st_size) {
		fprintf(stderr, "Fail in isinshm\n");
		exit(EXIT_FAILURE);
	}
	linkname[sb.st_size] = '\0';
	if((strncmp(linkname, str,strlen(str)))==0){
		return 1;
	}
	return 0;
}
int openfd_connect(char *programlocation,char *id,mode_t permission){
	srand(time(NULL));
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
						cmdlinefduric=malloc(strlen((dir->d_name))+strlen((dirb->d_name))+strlen("/proc//fd/")+1+5);
						strcpy(cmdlinefduric, "/proc/");
						strcat(cmdlinefduric, (dir->d_name));
						strcat(cmdlinefduric, "/fd/");
						strcat(cmdlinefduric, (dirb->d_name));
						struct stat sb;
						if(stat(cmdlinefduric, &sb)!=-1){
							if(S_ISREG(sb.st_mode)){
								if(isinshm(cmdlinefduric)){
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
										fd[currentcreatedmapindex] = open(location, O_RDWR);
										if (fd[currentcreatedmapindex] == -1) {
											foundfile=0;
										}

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
	currentcreatedmapindex=currentcreatedmapindex+1;
	return currentcreatedmapindex-1;
}
int openfd_create(char *programlocation,char *id,mode_t permission){
	srand(time(NULL));
		char strab[29]="";
		char *randomstring;
		randomstring=strab;
		sprintf(randomstring,"%i",(rand()));
		char* location;
		location=malloc(strlen(shmpath)+strlen(randomstring)+1);
		strcpy(location, shmpath);
		strcat(location, randomstring);
		fd[currentcreatedmapindex] = open(location, O_RDWR | O_CREAT, (mode_t) permission);
		if (fd[currentcreatedmapindex] == -1) {
			perror("Error opening shared memory");
			exit(EXIT_FAILURE);
		}
		char* locationbfgffsthf;
		locationbfgffsthf=malloc(strlen(shmpath)+strlen(randomstring)+1);
		strcpy(locationbfgffsthf, shmpath);
		strcat(locationbfgffsthf, randomstring);
		unlink(locationbfgffsthf);
	currentcreatedmapindex=currentcreatedmapindex+1;
	return currentcreatedmapindex-1;
}
int initshm(void){
	int lseekresult;
	lseekresult = lseek(fd[currentcreatedmapindex-1], 2*shmsize+25+((sharedstringsize+3)*sizeof(char)), SEEK_SET);
	if (lseekresult == -1) {
		close(fd[currentcreatedmapindex-1]);
		return -1;
	}
	lseekresult = write(fd[currentcreatedmapindex-1], "", 1);
	if (lseekresult != 1) {
		close(fd[currentcreatedmapindex-1]);
		return -1;
	}
	return 0;
}
void listmmaps(char *programlocation,char *foundmaps[],int maxmapfindnum){
			int foundmapcounter=0;
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
						cmdlinefduric=malloc(strlen((dir->d_name))+strlen((dirb->d_name))+strlen("/proc//fd/")+1+5);
						strcpy(cmdlinefduric, "/proc/");
						strcat(cmdlinefduric, (dir->d_name));
						strcat(cmdlinefduric, "/fd/");
						strcat(cmdlinefduric, (dirb->d_name));
						struct stat sb;
						if(stat(cmdlinefduric, &sb)!=-1){
							if(S_ISREG(sb.st_mode)){
								if(isinshm(cmdlinefduric)){
								if(searchb(cmdlinefduric,"luisvmffastmmapmq")){
									//char *cmdlinefduric is the file uri in /proc for an unlinked fastmmapmq adress owened by program with cmdline containing char *programlocation!
									//Lets check if this file contains char *id
									int intsearchmapcounter=0;
									int tempmmapfd=open(cmdlinefduric, O_RDONLY );
									char *tempmmap;
									tempmmap=mmap(0, shmsize, PROT_READ , MAP_SHARED, tempmmapfd, 0);
									foundmaps[foundmapcounter]=malloc(180);
									while(intsearchmapcounter<19){
										if(maxmapfindnum==foundmapcounter){
											foundfile=1;
											break;
										}
										foundmaps[foundmapcounter][intsearchmapcounter]=tempmmap[shmsize-40+intsearchmapcounter];
										intsearchmapcounter=intsearchmapcounter+1;
										foundfile=1;
									}
									munmap(tempmmap,shmsize);
									close(tempmmapfd);
									foundmapcounter=foundmapcounter+1;
								}
							}
						}
						}
				}
				}
				closedir(db);
				}
				}
			}
			}
			closedir(d);
			}
}
void creatememmap(void){
	map[currentcreatedmapindex-1] = mmap(0, shmsize+((sharedstringsize+3)*sizeof(char)), PROT_READ | PROT_WRITE, MAP_SHARED, fd[currentcreatedmapindex-1], 0);
	if (map[currentcreatedmapindex-1] == MAP_FAILED) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on shared memory mmap");
		exit(EXIT_FAILURE);
	}
}
int startmemmap(int create,char *programlocation,char *id, mode_t permission){
	int thismapindex=-1;
	if(create==1){
		thismapindex=openfd_create(programlocation,id,permission);
	}else{
		thismapindex=openfd_connect(programlocation,id,permission);
	}
	int openedshmstatus=initshm();
	int jjold=0;
	if(openedshmstatus==0){
		creatememmap();
		char strab[9]="";
		char *dataposb;
		dataposb=strab;
		indexb[currentcreatedmapindex-1]=0;
		if(map[currentcreatedmapindex-1][7]!='\x17'){
			jjold=0;
			while(jjold<=16){
				map[currentcreatedmapindex-1][jjold]='0';
				jjold=jjold+1;
			}
		}
		map[currentcreatedmapindex-1][7]='\x17';
		jjold=0;
		while(jjold<=6){
			dataposb[jjold]=map[currentcreatedmapindex-1][jjold];
			jjold=jjold+1;
		}
		indexb[currentcreatedmapindex-1]=0;
		jjold=0;
		while(jjold<=19){
			map[currentcreatedmapindex-1][shmsize-(40-jjold)]=id[jjold];
			jjold=jjold+1;
		}
		char* ididentfier="luisvmffastmmapmq\x17\x17\x17";
		jjold=0;
		while(jjold<=19){
			map[currentcreatedmapindex-1][shmsize-(20-jjold)]=ididentfier[jjold];
			jjold=jjold+1;
		}
		return thismapindex;
	}else{
		return -1;
	}
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
int writeutils(int writemapindexselect,  char *s) {
	int jjold=0;
	char stra[9]="";
	char straold[9]="";
	char *datapos;
	char *dataposold;
	datapos=stra;
	dataposold=straold;
	if(map[writemapindexselect][shmsize-42]=='A'){
		return -1;
	}
	if(map[writemapindexselect][shmsize-41]=='A'){
		return -1;
	}
	map[writemapindexselect][shmsize-42]='A';
	jjold=0;
	while(jjold<=6){
		datapos[jjold]=map[writemapindexselect][jjold];
		jjold=jjold+1;
	}
	jjold=0;
	while(jjold<=6){
		dataposold[jjold]=map[writemapindexselect][jjold+8];
		jjold=jjold+1;
	}
	int index=0;
	int indexreadold=0;
	int i=0;
	int lenscalc=strlen(s);
	index=atoi(datapos);
	indexreadold=atoi(dataposold);
	int writeupto=index+lenscalc+1;
	if(index!=0){
		writeupto=writeupto-17;
		if(writeupto>bufferlength-1){
			writeupto=(writeupto-(bufferlength-1))-1;
		}
		if((writeupto-lenscalc-1)<indexreadold){
			if((writeupto)>=indexreadold){
				map[writemapindexselect][shmsize-41]='\0';
				map[writemapindexselect][shmsize-42]='\0';
				return -1;
			}
		}
	}
	index=index+1;
	if(index==1){
		index=18;
	}
	i=index;
	int resetwriteposf=0;
	int oldiwrite=0;
	while(i<=lenscalc+index){
		if((i-1)<bufferlength+100)
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
			if((i-1)<bufferlength+100)
			map[writemapindexselect][i-1]=s[oldiwrite+i-index-18];
			i=i+1;
		}
	}
	if((i-2)<bufferlength+100)
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
	map[writemapindexselect][shmsize-41]='A';
	map[writemapindexselect][0]=vca+'0';
	map[writemapindexselect][1]=vcb+'0';
	map[writemapindexselect][2]=vcc+'0';
	map[writemapindexselect][3]=vcd+'0';
	map[writemapindexselect][4]=vce+'0';
	map[writemapindexselect][5]=vcf+'0';
	map[writemapindexselect][6]=vcg+'0';
	if(index>=bufferlength+18){
		jjold=0;
		while(jjold<=6){
			map[writemapindexselect][jjold]='0';
			jjold=jjold+1;
		}
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
	map[writemapindexselect][shmsize-41]='\0';
	map[writemapindexselect][shmsize-42]='\0';
   return 0;
}
char *readutils(int readmapindexselect,int gfifghdughfid) {
	int jjold=0;
	if(gfifghdughfid==0){
		char stra[maxmemreturnsize+100]="";
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		char *datapos;
		datapos=strab;
		int index=0;
		jjold=0;
		if(map[readmapindexselect][shmsize-41]=='A'){
			return "";
		}
		map[readmapindexselect][shmsize-41]='A';
		while(jjold<=6){
			datapos[jjold]=map[readmapindexselect][jjold];
			jjold=jjold+1;
		}
		map[readmapindexselect][shmsize-41]='\0';
		index=atoi(datapos);
		char *dataposb;
		dataposb=strab;
		indexb[readmapindexselect]=0;
		jjold=0;
		while(jjold<=6){
			dataposb[jjold]=map[readmapindexselect][jjold+8];
			jjold=jjold+1;
		}
		indexb[readmapindexselect]=atoi(dataposb);
		char *dataposbc;
		dataposbc=strab;
		sprintf(dataposbc,"%c%c",map[readmapindexselect][15],map[readmapindexselect][16]);
		int i=indexb[readmapindexselect]+17;
		if(indexb[readmapindexselect]==index-17){
			return "";
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				return "";
			}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<bufferlength+17){
				if((i)<bufferlength+100)
				tmpstr[i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				if((i)<bufferlength+100){
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[i-indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-indexb[readmapindexselect]-17;
			indexb[readmapindexselect]=0;
			i=indexb[readmapindexselect]+17;
			while(i<index){
				if((i)<bufferlength+100)
				tmpstr[offsetretarray+i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				if((i)<bufferlength+100){
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				if((i)<bufferlength+100)
				tmpstr[i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}
		indexb[readmapindexselect]=indexb[readmapindexselect]+i-indexb[readmapindexselect]-17;
		int vca=indexb[readmapindexselect]/1000000;
		int vcb=(indexb[readmapindexselect]-vca*1000000)/100000;
		int vcc=(indexb[readmapindexselect]-vca*1000000-vcb*100000)/10000;
		int vcd=(indexb[readmapindexselect]-vca*1000000-vcb*100000-vcc*10000)/1000;
		int vce=(indexb[readmapindexselect]-vca*1000000-vcb*100000-vcc*10000-vcd*1000)/100;
		int vcf=(indexb[readmapindexselect]-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100)/10;
		int vcg=(indexb[readmapindexselect]-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100-vcf*10);
		if(map[readmapindexselect][shmsize-41]=='A'){
			return "";
		}
		map[readmapindexselect][shmsize-41]='A';
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
		if(map[readmapindexselect][shmsize-41]=='A'){
			return "";
		}
		map[readmapindexselect][shmsize-41]='A';
		char stra[maxmemreturnsize+100]="";
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		char *datapos;
		datapos=strab;
		int index=0;
		jjold=0;
		while(jjold<=6){
			datapos[jjold]=map[readmapindexselect][jjold];
			jjold=jjold+1;
		}
		map[readmapindexselect][shmsize-41]='\0';
		index=atoi(datapos);
		char *dataposbc;
		dataposbc=strab;
		sprintf(dataposbc,"%c%c",map[readmapindexselect][15],map[readmapindexselect][16]);
		int i=indexb[readmapindexselect]+17;
		if(indexb[readmapindexselect]==index-17){
			map[readmapindexselect][shmsize-41]='\0';
			return "";
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				map[readmapindexselect][shmsize-41]='\0';
				return "";
			}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<bufferlength+17){
				if((i)<bufferlength+100)
				tmpstr[i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				if((i)<bufferlength+100){
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[i-indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-indexb[readmapindexselect]-17;
			indexb[readmapindexselect]=0;
			i=indexb[readmapindexselect]+17;
			while(i<index){
				if((i)<bufferlength+100)
				tmpstr[offsetretarray+i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				if((i)<bufferlength+100){
				if(map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				if((i)<bufferlength+100)
				tmpstr[i-indexb[readmapindexselect]-17]=map[readmapindexselect][i];
				i=i+1;
				if(i>=maxmemreturnsize+17){
					break;
				}
			}
		}
		indexb[readmapindexselect]=indexb[readmapindexselect]+i-indexb[readmapindexselect]-17;
		return tmpstr;
		}
	}
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
				mode_t permission=(mode_t)0000;
				nodeconretval=startmemmap(0,prog,strb,permission);
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
				char *prog;
				prog=malloc(strlen(str)+1);
				sprintf(prog,"%s",str);
				char *perm=strb;
				mode_t permission=(((perm[0]=='r')*4|(perm[1]=='w')*2|(perm[2]=='x'))<<6)|(((perm[3]=='r')*4|(perm[4]=='w')*2|(perm[5]=='x'))<<3)|(((perm[6]=='r')*4|(perm[7]=='w')*2|(perm[8]=='x')));
				nodeconretval=startmemmap(1,"None",prog,permission);
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
			callback->Call(1, argv);
		  }
	private:
		int nodesendretval=0;
		int valinputnodesend;
		std::string nodesentstring="";
		int nodelengthb;
};
class AsyncListWorker : public AsyncWorker {
	public:
		 AsyncListWorker(Callback *callback, std::string internalstring)
			:AsyncWorker(callback),nodeinternalstring(internalstring) {}
		 ~AsyncListWorker() {}
		  void Execute () {
				char *s=nodeinternalstring.c_str(); //Program
				int b=900; //Maximum number of mmaps to return on listmaps("string").
				char *retval[b];
				int ilist=0;
				while(ilist<=b){
					retval[ilist]=malloc(300);
					retval[ilist]="";
					ilist=ilist+1;
				}
				listmmaps(s,retval,b);
				char templist[900];
				char *retvalstr;
				retvalstr=templist;
				ilist=0;
				strcpy(retvalstr,retval[ilist]);
				ilist=ilist+1;
				while(ilist<b){
					if(strcmp(retval[ilist],"")!=0){
						strcat(retvalstr,",");
						strcat(retvalstr,retval[ilist]);
					}
					ilist=ilist+1;
				}
				nodereadretval=retvalstr;
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
		char *nodereadretval;
		std::string nodeinternalstring="";
};
class AsyncwritestringWorker : public AsyncWorker {
	public:
		 AsyncwritestringWorker(Callback *callback, int va, std::string internalstringb, int lengthb)
			:AsyncWorker(callback),valinputnodesend(va),nodesentstring(internalstringb),nodelengthb(lengthb) {}
		 ~AsyncwritestringWorker() {}
		  void Execute () {
				int vanode=valinputnodesend;
				int readmapindexselect=vanode;
				char stra[sharedstringsize+5]="";
				char *tmpstring="";
				tmpstring=stra;
				tmpstring=nodesentstring.c_str();
				int i=memmappedarraysize;
				while(i<memmappedarraysize+sharedstringsize){
					map[readmapindexselect][i]=tmpstring[i-memmappedarraysize];
					i=i+1;
					if(tmpstring[i-memmappedarraysize-1]=='\0'){
						break;
					}
				}
				map[readmapindexselect][i]='\0';
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
			int readmapindexselect=vanode;
			char stra[sharedstringsize+5]="";
			char *tmpstring="";
			tmpstring=stra;
			int i=memmappedarraysize;
			while(i<memmappedarraysize+sharedstringsize){
				tmpstring[i-memmappedarraysize]=map[readmapindexselect][i];
				if(map[readmapindexselect][i]=='\0'){
					break;
				}
				i=i+1;
			}
			nodereadretval=tmpstring;
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
	mode_t permission=(mode_t)0000;
	int nodeconretval=startmemmap(0,prog,internalstringb.c_str(),permission);
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
	mode_t permission=(((perm[0]=='r')*4|(perm[1]=='w')*2|(perm[2]=='x'))<<6)|(((perm[3]=='r')*4|(perm[4]=='w')*2|(perm[5]=='x'))<<3)|(((perm[6]=='r')*4|(perm[7]=='w')*2|(perm[8]=='x')));
	int nodeconretval=startmemmap(1,"None",prog,permission);
	info.GetReturnValue().Set(nodeconretval);
}
void writesync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
    v8::String::Utf8Value param2(info[1]->ToString());
    std::string internalstringbnode = std::string(*param2);  
	info.GetReturnValue().Set(writeutils(vanode,internalstringbnode.c_str()));
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
	while(i<memmappedarraysize+sharedstringsize){
		map[readmapindexselect][i]=tmpstring[i-memmappedarraysize];
		i=i+1;
		if(tmpstring[i-memmappedarraysize-1]=='\0'){
			break;
		}
	}
	map[readmapindexselect][i]='\0';
	info.GetReturnValue().Set(0);
}
void getstringsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
	int readmapindexselect=vanode;
	char stra[sharedstringsize+5]="";
	char *tmpstring="";
	tmpstring=stra;
	int i=memmappedarraysize;
	while(i<memmappedarraysize+sharedstringsize){
		tmpstring[i-memmappedarraysize]=map[readmapindexselect][i];
		if(map[readmapindexselect][i]=='\0'){
			break;
		}
		i=i+1;
	}
	info.GetReturnValue().Set(Nan::New(tmpstring).ToLocalChecked());
}
void readsync(const FunctionCallbackInfo<Value>& info) {
	int vanode=info[0]->NumberValue();
	int vbnode=info[1]->NumberValue();
	char *readsyncretval=readutils(vanode,vbnode);
	info.GetReturnValue().Set(Nan::New(readsyncretval).ToLocalChecked());
}
void listsync(const FunctionCallbackInfo<Value>& info) {
	char *s;
	int b=900; //Maximum number of mmaps to return on listmaps("string").
	v8::String::Utf8Value param1(info[0]->ToString());
	std::string internalstring = std::string(*param1);
	s=internalstring.c_str();
	char *retval[b];
	int ilist=0;
	while(ilist<=b){
		retval[ilist]=malloc(300);
		retval[ilist]="";
		ilist=ilist+1;
	}
	listmmaps(s,retval,b);
	char templist[900];
	char *retvalstr;
	retvalstr=templist;
	ilist=0;
	strcpy(retvalstr,retval[ilist]);
	ilist=ilist+1;
	while(ilist<b){
		if(strcmp(retval[ilist],"")!=0){
			strcat(retvalstr,",");
			strcat(retvalstr,retval[ilist]);
		}
		ilist=ilist+1;
	}
	info.GetReturnValue().Set(Nan::New(retvalstr).ToLocalChecked());
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
NAN_METHOD(listmmapasync) {
    v8::String::Utf8Value param1(info[0]->ToString());
    std::string internalstring = std::string(*param1);  
	Callback *callback = new Callback(info[1].As<Function>());
	AsyncQueueWorker(new AsyncListWorker(callback,internalstring));
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
	Nan::Set(target, Nan::New("ListAsync").ToLocalChecked(),
		Nan::GetFunction(Nan::New<FunctionTemplate>(listmmapasync)).ToLocalChecked());
	NODE_SET_METHOD(target, "ConnectMmapSync", consync);
	NODE_SET_METHOD(target, "CreateMmapSync", createsync);
	NODE_SET_METHOD(target, "WriteSync", writesync);
	NODE_SET_METHOD(target, "ReadSync", readsync);
	NODE_SET_METHOD(target, "ListSync", listsync);
	NODE_SET_METHOD(target, "GetSharedStringSync", getstringsync);
	NODE_SET_METHOD(target, "WriteSharedStringSync", writestringsync);
}
NODE_MODULE(NativeExtension, InitAll)
