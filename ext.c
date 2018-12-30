#include <Python.h>
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
	char *str=shmfolder;
	int retryisinshm=10;
	int continueisinshm=0;
	while(retryisinshm>0){
		continueisinshm=0;
		if (lstat(fdlink, &sb) == -1) {
			continueisinshm=-1;
		}
		linkname = malloc(sb.st_size + 1);
		if (linkname == NULL) {
			continueisinshm=-1;
		}
		r = readlink(fdlink, linkname, sb.st_size + 1);
		if (r < 0) {
			continueisinshm=-1;
		}
		if (r > sb.st_size) {
			continueisinshm=-1;
		}
		retryisinshm=retryisinshm-1;
		if(continueisinshm==0){
			break;
		}else{
			sleep(0.01);
		}
	}
	if(continueisinshm==-1){
		return 0;
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
		if(map[currentcreatedmapindex-1][7]!='\x21'){
			jjold=0;
			while(jjold<=16){
				map[currentcreatedmapindex-1][jjold]='0';
				jjold=jjold+1;
			}
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
static PyObject* writemessage(PyObject* self,  PyObject *args) {
	int jjold=0;
	int writemapindexselect=0;
	char *s;
	char stra[9]="";
	char straold[9]="";
	char *datapos;
	char *dataposold;
	datapos=stra;
	dataposold=straold;
	if (!PyArg_ParseTuple(args, "is", &writemapindexselect, &s)) {
		return NULL;
	}
	if(writemapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(writemapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	while(map[writemapindexselect][shmsize-42]=='A'){}
	while(map[writemapindexselect][shmsize-41]=='A'){}
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
	if(map[writemapindexselect][7]=='\x17'){
	if(index!=0){
		writeupto=writeupto-17;
		if(writeupto>bufferlength-1){
			writeupto=(writeupto-(bufferlength-1))-1;
		}
		if((writeupto-lenscalc-1)<indexreadold){
			if((writeupto)>=indexreadold){
				map[writemapindexselect][shmsize-41]='\0';
				map[writemapindexselect][shmsize-42]='\0';
				return Py_BuildValue("i", -1);
			}
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
   return Py_BuildValue("i", 0);
}
static PyObject* readmessage(PyObject* self,  PyObject *args) {
	int readmapindexselect=0;
	int gfifghdughfid=0;
	int jjold=0;
	if (!PyArg_ParseTuple(args, "ii", &readmapindexselect, &gfifghdughfid)) {
		return NULL;
	}
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
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
			return Py_BuildValue("s", "");
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
			return Py_BuildValue("s","");
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				return Py_BuildValue("s","");
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
			return Py_BuildValue("s", "");
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
		return Py_BuildValue("s",tmpstr );
		}
	}else{
		map[readmapindexselect][7]='\x21';
		if(map[readmapindexselect][shmsize-41]=='A'){
			return Py_BuildValue("s", "");
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
			return Py_BuildValue("s","");
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				map[readmapindexselect][shmsize-41]='\0';
				return Py_BuildValue("s","");
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
		return Py_BuildValue("s",tmpstr );
		}
	}
}
static PyObject* pyinitmmap(PyObject* self,  PyObject *args) {
	char *s;
	char *b;
	char *prog;
	if (!PyArg_ParseTuple(args, "ss",&b,&s)) {
		return NULL;
	}
	mode_t permission=(mode_t)0000;
	prog=malloc(strlen(b)+1);
	sprintf(prog,"%s",b);
	return Py_BuildValue("i", startmemmap(0,prog,s,permission));
}
static PyObject* pygetsharedstring(PyObject* self,  PyObject *args) {
	int readmapindexselect=-1;
	char stra[sharedstringsize+5]="";
	char *tmpstring="";
	tmpstring=stra;
	if (!PyArg_ParseTuple(args, "i",&readmapindexselect)) {
		return NULL;
	}
	int i=memmappedarraysize;
	while(i<memmappedarraysize+sharedstringsize){
		tmpstring[i-memmappedarraysize]=map[readmapindexselect][i];
		if(map[readmapindexselect][i]=='\0'){
			break;
		}
		i=i+1;
	}
	return Py_BuildValue("s", tmpstring);
}
static PyObject* pywritesharedstring(PyObject* self,  PyObject *args) {
	int readmapindexselect=-1;
	char stra[sharedstringsize+5]="";
	char *tmpstring="";
	tmpstring=stra;
	if (!PyArg_ParseTuple(args, "is",&readmapindexselect,&tmpstring)) {
		return NULL;
	}
	int i=memmappedarraysize;
	while(i<memmappedarraysize+sharedstringsize){
		map[readmapindexselect][i]=tmpstring[i-memmappedarraysize];
		i=i+1;
		if(tmpstring[i-memmappedarraysize-1]=='\0'){
			break;
		}
	}
	map[readmapindexselect][i]='\0';
	return Py_BuildValue("i", 0);
}
static PyObject* pyinitmmap_create(PyObject* self,  PyObject *args) {
	char *s;
	char *b;
	char *prog;
	char *perm;
	if (!PyArg_ParseTuple(args, "ss",&s,&perm)) {
		return NULL;
	}
	b="None";
	mode_t permission=(((perm[0]=='r')*4|(perm[1]=='w')*2|(perm[2]=='x'))<<6)|(((perm[3]=='r')*4|(perm[4]=='w')*2|(perm[5]=='x'))<<3)|(((perm[6]=='r')*4|(perm[7]=='w')*2|(perm[8]=='x')));
	prog=malloc(strlen(b)+1);
	sprintf(prog,"%s",b);
	return Py_BuildValue("i", startmemmap(1,prog,s,permission));
}
static PyObject* pylistmap(PyObject* self,  PyObject *args) {
	char *s;
	int b=900; //Maximum number of mmaps to return on listmaps("string").
	if (!PyArg_ParseTuple(args, "s",&s)) {
		return NULL;
	}
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
	return Py_BuildValue("s",retvalstr);
}
static char mmap_docs_write[] =
   "write(id,'data'): Write 'data' into id message queue. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_read[] =
"read(id,bool): Read data from id message queue. Pass 0 to erase data when read so that other processes using read(id,0) can't see it.\n \
Pass 1 to read but don't erase to other processes using read(id,0) or read(id,1), just erase for current process.\n \
When using read(id,0) first read can get data written before this process started. First read(id,0) reads all data written to id and not read yet.\
First read(id,1) only reads data written after this proccess has called connected. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_connect[] =
   "connectmmap('filepath','id'): Connects to write/read messages from this id created by process in filepath.\
The id can contain any characters valid in a file name (no / or null character).This function must be called before write() or read().Returns mmapid.\n";
static char mmap_docs_create[] =
   "createmmap('id','permissions'): Creates a mmap with id 'id' and permissions.\nExample of permission 'rwx------' or 'rwxrwxrwx'. If the user has no write and read permissions on the mmap the program will segfault.";
static char mmap_docs_get_shared[] =
   "getsharedstring(id): Get a shared string that is avaliable to all programs connected to this mmap. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_write_shared[] =
   "writesharedstring(id,string): Writes a shared string thet is avaliable to all programs conncted to this mmap. id should be the value returned by connectmmap() or createmmap().";
static char mmap_docs_list[] =
   "listmaps('filepath'): List all mmap ids avaliable from the program specified in 'filepath' string. 'filepath' doesn't need to contain full location of program, the function can list partial matches. Command line arguments can also be present in 'filepath'.";
static PyMethodDef mmap_funcs[] = {
	{"write", (PyCFunction)writemessage, METH_VARARGS, mmap_docs_write},
	{"read", (PyCFunction)readmessage, METH_VARARGS, mmap_docs_read},
	{"connectmmap", (PyCFunction)pyinitmmap, METH_VARARGS, mmap_docs_connect},
	{"createmmap", (PyCFunction)pyinitmmap_create, METH_VARARGS, mmap_docs_create},
	{"getsharedstring", (PyCFunction)pygetsharedstring, METH_VARARGS, mmap_docs_get_shared},
	{"writesharedstring", (PyCFunction)pywritesharedstring, METH_VARARGS, mmap_docs_write_shared},
	{"listmaps", (PyCFunction)pylistmap, METH_VARARGS, mmap_docs_list},
	{ NULL, NULL, 0, NULL}
};
void initfastmmap(void) {
	Py_InitModule3("fastmmap", mmap_funcs,"Fast IPC with mmap");
}
