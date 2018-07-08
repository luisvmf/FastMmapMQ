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
int initshm(void){
	int lseekresult;
	lseekresult = lseek(fd[currentcreatedmapindex-1], shmsize-1, SEEK_SET);
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
									int intsearchmapcounter=0;
									int tempmmapfd=open(cmdlinefduric, O_RDWR | O_CREAT, (mode_t)0600);
									char *tempmmap;
									tempmmap=mmap(0, shmsize, PROT_READ | PROT_WRITE, MAP_SHARED, tempmmapfd, 0);
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
				closedir(db);
				}
				}
			}
			}
			closedir(d);
			}
}
void creatememmap(void){
	map[currentcreatedmapindex-1] = mmap(0, shmsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd[currentcreatedmapindex-1], 0);
	if (map[currentcreatedmapindex-1] == MAP_FAILED) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on shared memory mmap");
		exit(EXIT_FAILURE);
	}
}
int startmemmap(char *programlocation,char *id){
	int thismapindex=openfd(programlocation,id);
	int openedshmstatus=initshm();
	if(openedshmstatus==0){
		creatememmap();
		char strab[9]="";
		char *dataposb;
		dataposb=strab;
		indexb[currentcreatedmapindex-1]=0;
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
		indexb[currentcreatedmapindex-1]=0;
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
int writemmap(int writemapindexselect,  char *s) {
	char stra[9]="";
	char *datapos;
	datapos=stra;
	//The loop below is sometimes optimized by the compiler and the program crashes???
	//This is to avoid race between multiple threads.
	while(map[writemapindexselect][shmsize-42]=='A'){
	}
	while(map[writemapindexselect][shmsize-41]=='A'){
	}
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
	map[writemapindexselect][shmsize-42]='A';
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
char *readmmap(int readmapindexselect,int gfifghdughfid) {
	if(gfifghdughfid==0){
		//The loop below is sometimes optimized by the compiler and the program crashes???
		//This is to avoid race between multiple threads.
		while(map[readmapindexselect][shmsize-41]=='A'){
		}
		while(map[readmapindexselect][shmsize-42]=='A'){
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
		indexb[readmapindexselect]=0;
		dataposb[0]=map[readmapindexselect][8];
		dataposb[1]=map[readmapindexselect][9];
		dataposb[2]=map[readmapindexselect][10];
		dataposb[3]=map[readmapindexselect][11];
		dataposb[4]=map[readmapindexselect][12];
		dataposb[5]=map[readmapindexselect][13];
		dataposb[6]=map[readmapindexselect][14];
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
		while(map[readmapindexselect][shmsize-42]=='A'){
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
		return tmpstr;
		}
	}
}
int connectmmap(char *b,char *s) {
	char *prog;
	prog=malloc(strlen(b)+1);
	sprintf(prog,"%s",b);
	return startmemmap(prog,s);
}
