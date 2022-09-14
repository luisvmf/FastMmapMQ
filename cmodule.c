//---------------------------------------------------------
//---------------------------------------------------------
//Copyright (c) 2022 Luís Victor Muller Fabris. Apache License.
//---------------------------------------------------------
//---------------------------------------------------------
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
#include <sys/file.h>
#ifndef __cplusplus
	#include <stdatomic.h>
#endif
#include <stdint.h>
#include <errno.h>


//--------------------------------------------------------
//--------------------------------------------------------
//mmaped file structure:
//--------------------------------------------------------
// map[mapindex][i]=| 0 | 1 | 2 | 3  | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |  15   |    16     | 17 | 18 | 19 | ...............|  shmsize-42  |shmsize-41|..............
//                  |uint32_t w_index|uint32_t futex |uint32_t r_index |uint32_t reset_counter| mmap State| circular buffer data .........| Locking type |Future use| Shared string
//--------------------------------------------------------
//--------------------------------------------------------

//---------------------------------------------------------
//---------------------------------------------------------
// Library configurations, use the same value on all FastMmapMQ instances that may connect with each other.
//---------------------------------------------------------
//---------------------------------------------------------
#define futexavaliable //Comment this line if this system doesn't support futex, the library will then fallback to a spinlock.
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------




//XXX TODO freebsd support, futex and connection without procfs.

#ifdef futexavaliable
	#include <sys/syscall.h> //XXX TODO Check on freebsd
	#include <linux/futex.h> //XXX TODO Check on freebsd
#endif

//XXX TODO put the global variables on a struct, add better function names to prevent collisions.

//Constants. FastMmapMQ has been validated to work using only the constants defined bellow. Changing this may cause errors. This constants should be the same for all programs using a mmap. Diferent constants for the same mmap will result in undefined behavior.
#define shmpath "/dev/shm/luisvmfcomfast2mapmqshm-"
#define bufferlength  (999999) //Number of characters in buffer. Maximum value is 999999.
#define maxmemreturnsize  (999999) //Maximum number of characters returned by read function in one run. Maximum value is 999999.
#define sharedstringsize  (999999) //Maximum number of characters in shared string. No maximum.
#define maxfdnum 80
#define memmappedarraysize  (bufferlength+100)
#define shmsize (memmappedarraysize * sizeof(char))
int fd[bufferlength]={-1};
volatile uint8_t *map[bufferlength+200];
int currentcreatedmapindex=0;
int indexb[bufferlength]={0};
uint32_t *futexpointers[bufferlength+200]={0};
int errdisplayed=0;

static int futex(uint32_t *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3){
	#ifdef futexavaliable
		return syscall(SYS_futex, uaddr, futex_op, val,timeout, uaddr, val3);
	#endif
}
int atomiccomparefastmmap(uint32_t *a, const uint32_t *b, int c){
#ifdef __cplusplus
	return __sync_bool_compare_and_swap(a,*b,c);
#else
	return atomic_compare_exchange_strong(a,b,c);
#endif
}

//Aquire the lock, lockmechanism=0 for futex+spinlock and lockmechanism=1 for flock. Futex lock is faster but a deadlock may occur if one of the processes is terminated while a writemmap/ readmmap call is on progress.
//If the programs doing the read/write are independent, it is recommended using the flock, unless performance is critical.
static void lockfastmmapmq(uint32_t *futexp,uint32_t lockmechanism, int mmapfd){
	if(lockmechanism==0){
		int s;
		while(1){
			const uint32_t one = 1;
			const uint32_t zero = 0;
			int i=0;
			#ifndef futexavaliable
				if(errdisplayed==0){
					perror("Warning, this version of FastMmapMQ is built without futex support, but a futex was required on mmap initialization.\nFalling back to spinlock, this may cause above than normal CPU usage");
					errdisplayed=1;
				}
			#endif
			if(atomiccomparefastmmap(futexp, &one, 0)){
				return;
			}
			while(i<10){
				if(*futexp==0){
					break;
				}
				i++;
			}
			if(atomiccomparefastmmap(futexp, &one, 0)){
				return;
			}
			#ifdef futexavaliable
				if(atomiccomparefastmmap(futexp, &zero, 2)){
					s = futex(futexp, FUTEX_WAIT, 2, NULL, NULL, 0);// Wait to aquire the lock.
					if(s==-1 && errno!=EAGAIN){
						perror("Error at lockfutex");
						exit(EXIT_FAILURE);
					}
				}
			#endif
		}
	}else{
		//printf("flock");
		int flockres=flock(mmapfd,LOCK_EX);
		if(flockres==-1){
			perror("flock(fd,LOCK_EX) failled");
			return;
		}
	}
}
//Release the lock, lockmechanism=0 for futex+spinlock and lockmechanism=1 for flock. Futex lock is faster but a deadlock may occur if one of the processes is terminated while a writemmap/ readmmap call is on progress.
//If the programs doing the read/write are independent, it is recommended using the flock, unless performance is critical.
static void unlockfastmmapmq(uint32_t *futexp,uint32_t lockmechanism, int mmapfd){
	if(lockmechanism==0){
		int s;
		const uint32_t zero = 0;
		const uint32_t two = 2;
		if(atomiccomparefastmmap(futexp, &zero, 1)){
			return;	
		}
		#ifdef futexavaliable
		if(atomiccomparefastmmap(futexp, &two, 1)){
			s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);//Wake the other process. 
			if(s==-1){
				perror("Error at releasefutex");
			}
		}
		#endif
	}else{
		//printf("flock");
		int flockres=flock(mmapfd,LOCK_UN);
		if(flockres==-1){
			perror("flock(fd,LOCK_UN) failled");
			return;
		}
	}
}


int search(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	int isearch=0;
	char temp[650];
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
			if(fp) {
				fclose(fp);
			}
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
int isinshm(char *fdlink,char *id){
	struct stat sb;
	char *linkname;
	ssize_t r;
	char *str;
	str=(char *)malloc(strlen(shmpath)+strlen(id)+3);
	strcpy(str, shmpath);
	strcat(str, id);
	strcat(str, "-");
	int retryisinshm=10;
	int continueisinshm=0;
	while(retryisinshm>0){
		continueisinshm=0;
		if (lstat(fdlink, &sb) == -1) {
			continueisinshm=-1;
		}
		linkname = (char *)malloc(sb.st_size + 1000+100);
		if (linkname == NULL) {
			continueisinshm=-1;
		}
		r = readlink(fdlink, linkname, sb.st_size +1000+30);
		if (r < 0) {
			continueisinshm=-1;
		}
		if (r > sb.st_size+1000) {
			continueisinshm=-1;
		}
		retryisinshm=retryisinshm-1;
		if(continueisinshm==0){
			break;
		}else{
			usleep(0.05*1000000);
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
				cmdlineuri=(char *)malloc(strlen((dir->d_name))+strlen("/proc//cmdline")+1);
				strcpy(cmdlineuri, "/proc/");
				strcat(cmdlineuri, (dir->d_name));
				strcat(cmdlineuri, "/cmdline");
				if(search(cmdlineuri,programlocation)){
				char *cmdlinefduri;
				cmdlinefduri=(char *)malloc(strlen((dir->d_name))+strlen("/proc//fd/")+1);
				strcpy(cmdlinefduri, "/proc/");
				strcat(cmdlinefduri, (dir->d_name));
				strcat(cmdlinefduri, "/fd/");
				DIR *db;
				struct dirent *dirb;
				db=opendir(cmdlinefduri);
				if(db){
				int curfdnum=0;
				while((dirb=readdir(db))!=NULL){
				int tmpnumb = atoi(dirb->d_name);
				if(tmpnumb==0&&(dirb->d_name)[0]!='0'){}else{
						if(curfdnum>maxfdnum){
							break;
						}
						curfdnum=curfdnum+1;
						char *cmdlinefduric;
						cmdlinefduric=(char *)malloc(strlen((dir->d_name))+strlen((dirb->d_name))+strlen("/proc//fd/")+1+5);
						strcpy(cmdlinefduric, "/proc/");
						strcat(cmdlinefduric, (dir->d_name));
						strcat(cmdlinefduric, "/fd/");
						strcat(cmdlinefduric, (dirb->d_name));
						struct stat sb;
						if(stat(cmdlinefduric, &sb)!=-1){
							if(S_ISREG(sb.st_mode)){
								if(isinshm(cmdlinefduric,id)){
										foundfile=1;
										char* location;
										location=(char *)malloc(strlen(shmpath)+strlen(id)+1+strlen(cmdlinefduric));
										strcpy(location, cmdlinefduric);
										fd[currentcreatedmapindex] = open(location, O_RDWR);
										if (fd[currentcreatedmapindex] == -1) {
											foundfile=0;
											free(location);
											free(cmdlinefduri);
											free(cmdlinefduric);
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
				free(cmdlineuri);
			}
			}
			closedir(d);
			}
	if(foundfile==0){
		return -1;
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
		location=(char *)malloc(strlen(shmpath)+strlen(randomstring)+strlen(id)+5);
		strcpy(location, shmpath);
		strcat(location, id);
		strcat(location, "-");
		strcat(location, randomstring);
		fd[currentcreatedmapindex] = open(location, O_RDWR | O_CREAT, (mode_t) permission);
		if (fd[currentcreatedmapindex] == -1) {
			perror("Error opening shared memory");
			exit(EXIT_FAILURE);
		}
		char* locationbfgffsthf;
		locationbfgffsthf=(char *)malloc(strlen(shmpath)+strlen(randomstring)+1);
		strcpy(locationbfgffsthf, shmpath);
		strcat(locationbfgffsthf, randomstring);
		unlink(location);
	currentcreatedmapindex=currentcreatedmapindex+1;
	return currentcreatedmapindex-1;
}
int initshm(void){
	int lseekresult;
	lseekresult = lseek(fd[currentcreatedmapindex-1], 2*shmsize+25+((sharedstringsize+3)*sizeof(char)), SEEK_SET);
	if (lseekresult == -1) {
		perror("lseek1");
		close(fd[currentcreatedmapindex-1]);
		return -1;
	}
	lseekresult = write(fd[currentcreatedmapindex-1], "", 1);
	if (lseekresult != 1) {
		perror("lseek2");
		close(fd[currentcreatedmapindex-1]);
		return -1;
	}
	return 0;
}
int creatememmap(void){
	if(fd[currentcreatedmapindex-1]==-1){
		return -1;
	}
	map[currentcreatedmapindex-1] = (volatile uint8_t *)mmap(0, shmsize+((sharedstringsize+3)*sizeof(char)), PROT_READ | PROT_WRITE, MAP_SHARED, fd[currentcreatedmapindex-1], 0);
	if (map[currentcreatedmapindex-1] == MAP_FAILED) {
		close(fd[currentcreatedmapindex-1]);
		perror("Error on shared memory mmap");
		return -1;
	}
	futexpointers[currentcreatedmapindex-1]= (uint32_t*)(&(map[currentcreatedmapindex-1][4]));
	return 0;
}
int startmemmap(int create,char *programlocation,char *id, mode_t permission,int locking){
	if(currentcreatedmapindex>bufferlength-5){
		perror("Maximum mmap number exceeded");
		exit(EXIT_FAILURE);
	}
	int thismapindex=-1;
	int openedshmstatus=1;
	if(create==1){
		thismapindex=openfd_create(programlocation,id,permission);
		openedshmstatus=initshm();
	}else{
		thismapindex=openfd_connect(programlocation,id,permission);
		openedshmstatus=0;
	}
	int jjold=0;
	if(thismapindex==-1){
		free(programlocation);
		return -1;
	}
	if(creatememmap()==-1){
		free(programlocation);
		return -1;
	}
	if(openedshmstatus==0){
		*futexpointers[currentcreatedmapindex-1]=1;//Start futex unlocked.
		char strab[9]="";
		indexb[currentcreatedmapindex-1]=0;
		if(map[currentcreatedmapindex-1][16]!='\x17'){
		if(map[currentcreatedmapindex-1][16]!='\x21'){
			uint32_t *indexaux1=(uint32_t*)(&(map[currentcreatedmapindex-1][0]));
			*indexaux1=0;
			uint32_t *indexaux2=(uint32_t*)(&(map[currentcreatedmapindex-1][8]));
			*indexaux2=0;
			uint32_t *indexaux3=(uint32_t*)(&(map[currentcreatedmapindex-1][12]));
			*indexaux3=0;
			if(locking==1){
				map[currentcreatedmapindex-1][shmsize-42]='A';
			}else{
				map[currentcreatedmapindex-1][shmsize-42]='B';
			}
			map[currentcreatedmapindex-1][16]='0';
		}
		}
		map[currentcreatedmapindex-1][16]='\x17';
		jjold=0;
		indexb[currentcreatedmapindex-1]=0;
		jjold=0;
		while(jjold<=19){
			map[currentcreatedmapindex-1][shmsize-(40-jjold)]=id[jjold];
			jjold=jjold+1;
			if(id[jjold-1]=='\0')
				break;
		}
		while(jjold<=19){
			map[currentcreatedmapindex-1][shmsize-(40-jjold)]='\0';
			jjold=jjold+1;		
		}
		char* ididentfier=(char*)"luisvmffastmmapmq\x17\x17\x17";
		jjold=0;
		while(jjold<=19){
			map[currentcreatedmapindex-1][shmsize-(20-jjold)]=ididentfier[jjold];
			jjold=jjold+1;
		}
		return thismapindex;
	}else{
		free(programlocation);
		return -1;
	}
}
void addresetcounter(int thismapindexreset){
	uint32_t *indexaux1=(uint32_t*)(&(map[thismapindexreset][12]));
	*indexaux1=*indexaux1+1;
}
int getresetcounter(int thismapindexreset){
	uint32_t *indexaux1=(uint32_t*)(&(map[thismapindexreset][12]));
	int aux=(int)*indexaux1;
	return aux;
}
int writemmap(int writemapindexselect,  char *s) {
	int jjold=0;
	char stra[9]="";
	char straold[9]="";
	if(writemapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(writemapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux;
	 lockingaux=0;
	if(map[writemapindexselect][shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(futexpointers[writemapindexselect],lockingaux,fd[writemapindexselect]);
	jjold=0;
	uint32_t index=0;
	uint32_t indexreadold=0;
	int i=0;
	int lenscalc=strlen(s);
	uint32_t *indexaux1=(uint32_t*)(&(map[writemapindexselect][0]));
	index=*indexaux1;
	uint32_t *indexaux2=(uint32_t*)(&(map[writemapindexselect][8]));
	indexreadold=*indexaux2;
	int writeupto=index+lenscalc+1;
	if(map[writemapindexselect][16]=='\x17'){
	if(index!=0){
		writeupto=writeupto-17;
		if(writeupto>bufferlength-1){
			writeupto=(writeupto-(bufferlength-1))-1;
		}
		if((writeupto-lenscalc-1)<indexreadold){
			if((writeupto)>=indexreadold){
				unlockfastmmapmq(futexpointers[writemapindexselect],lockingaux,fd[writemapindexselect]);
				return -1;
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
	uint32_t *pointerindex=(uint32_t*)(&(map[writemapindexselect][0]));
	*pointerindex=index;
	if(index>=bufferlength+18){
		*pointerindex=0;
		addresetcounter(writemapindexselect);
	}
	unlockfastmmapmq(futexpointers[writemapindexselect],lockingaux,fd[writemapindexselect]);
   return 0;
}
char *readmmap(int readmapindexselect,int gfifghdughfid) {
	int jjold=0;
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux;
	lockingaux=0;
	if(map[readmapindexselect][shmsize-42]=='A'){
		lockingaux=1;
	}
	if(gfifghdughfid==0){
		char *stra=(char *)malloc((maxmemreturnsize+100)*sizeof(char));
		if(stra==NULL){
			perror("Malloc fail on readmmap");
			exit(EXIT_FAILURE);
		}
		int n=0;
		while(n<(maxmemreturnsize+100)){
			stra[n]='\0';
			n++;
		}
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		uint32_t index=0;
		jjold=0;
		lockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		uint32_t *indexauxaa=(uint32_t*)(&(map[readmapindexselect][0]));
		index=*indexauxaa;
		indexb[readmapindexselect]=0;
		jjold=0;
		uint32_t *indexauxbb=(uint32_t*)(&(map[readmapindexselect][8]));
		indexb[readmapindexselect]=(int)*indexauxbb;
		unlockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		int i=indexb[readmapindexselect]+17;
		if(indexb[readmapindexselect]==index-17){
			tmpstr[0]='\0';
			return tmpstr;
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				tmpstr[0]='\0';
				return tmpstr;
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
		lockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		uint32_t *pointerindex=(uint32_t*)(&(map[readmapindexselect][8]));
		*pointerindex=indexb[readmapindexselect];
		unlockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		return tmpstr;
		}
	}else{
		map[readmapindexselect][16]='\x21';
		lockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		char *stra=(char *)malloc((maxmemreturnsize+100)*sizeof(char));
		if(stra==NULL){
			perror("Malloc fail on readmmap");
			exit(EXIT_FAILURE);
		}
		int n=0;
		while(n<(maxmemreturnsize+100)){
			stra[n]='\0';
			n++;
		}
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		int index=0;
		uint32_t *pointerindex=(uint32_t*)(&(map[readmapindexselect][0]));
		index=(int)*pointerindex;
		unlockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
		int i=indexb[readmapindexselect]+17;
		if(indexb[readmapindexselect]==index-17){
			tmpstr[0]='\0';
			return tmpstr;
		}else{
		if(indexb[readmapindexselect]==index){
			if(index==0){
				tmpstr[0]='\0';
				return tmpstr;
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
	prog=(char *)malloc(strlen(b)+1);
	sprintf(prog,"%s",b);
	mode_t permission=(mode_t)0000;
	return startmemmap(0,prog,s,permission,1);
}
int createmmap(char *b,char *s,int locking) {
	char *prog;
	prog=(char *)malloc(strlen(b)+1);
	char *n=(char*)"None";
	sprintf(prog,"%s",n);
	char *perm=s;
	mode_t permission=(((perm[0]=='r')*4|(perm[1]=='w')*2|(perm[2]=='x'))<<6)|(((perm[3]=='r')*4|(perm[4]=='w')*2|(perm[5]=='x'))<<3)|(((perm[6]=='r')*4|(perm[7]=='w')*2|(perm[8]=='x')));
	int iaux=startmemmap(1,prog,b,permission,locking);
	return iaux;
}
char *getsharedstring(int readmapindexselect) {
	char *tmpstring=(char *)malloc(sharedstringsize+5);
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(map[readmapindexselect][shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
	int i=memmappedarraysize;
	while(i<memmappedarraysize+sharedstringsize){
		tmpstring[i-memmappedarraysize]=map[readmapindexselect][i];
		if(map[readmapindexselect][i]=='\0'){
			break;
		}
		i=i+1;
	}
	unlockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
	return tmpstring;
}
int writesharedstring(int readmapindexselect,char* tmpstring) {
	char stra[sharedstringsize+5]="";
	if(readmapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(map[readmapindexselect][shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
	int i=memmappedarraysize;
	while(i<memmappedarraysize+sharedstringsize){
		map[readmapindexselect][i]=tmpstring[i-memmappedarraysize];
		i=i+1;
		if(tmpstring[i-memmappedarraysize-1]=='\0'){
			break;
		}
	}
	unlockfastmmapmq(futexpointers[readmapindexselect],lockingaux,fd[readmapindexselect]);
	return 0;
}
