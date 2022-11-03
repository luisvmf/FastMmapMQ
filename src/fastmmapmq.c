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

//XXX TODO freebsd support, futex and connection without procfs.
//XXX Test data loss.

//---------------------------------------------------------
//---------------------------------------------------------
// Library configurations, use the same value on all FastMmapMQ instances that may connect with each other.
//---------------------------------------------------------
//---------------------------------------------------------
//Constants. FastMmapMQ has been validated to work using only the constants defined bellow. Changing this may cause errors. This constants should be the same for all programs using a mmap. Diferent constants for the same mmap will result in undefined behavior.
#define fastmmapmq_shmpath "/dev/shm/luisvmfcomfast3mapmqshm-"
#define fastmmapmq_bufferlength  (999999) //Number of characters in buffer. Maximum value is 999999.
#define fastmmapmq_maxmemreturnsize  (999999) //Maximum number of characters returned by read function in one run. Maximum value is 999999.
#define fastmmapmq_sharedstringsize  (999999) //Maximum number of characters in shared string. No maximum.
#define fastmmapmq_maxfdnum 80
#define fastmmapmq_futexavaliable //Comment this line if this system doesn't support futex, the library will then fallback to a spinlock.
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

#ifdef fastmmapmq_futexavaliable
	#include <sys/syscall.h> //XXX TODO Check on freebsd
	#include <linux/futex.h> //XXX TODO Check on freebsd
#endif

//--------------------------------------------------------
//--------------------------------------------------------
//mmaped file structure:
//--------------------------------------------------------
// map[mapindex][i]=| 0 | 1 | 2 | 3  | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |  15   |    16     | 17 | 18 | 19 | ...............|  shmsize-42  |shmsize-41|..............
//                  |uint32_t w_index|uint32_t futex |uint32_t r_index |uint32_t reset_counter| mmap State| circular buffer data .........| Locking type |Future use| Shared string
//--------------------------------------------------------
//--------------------------------------------------------

typedef struct{
	int memmappedarraysize;
	int shmsize;
	int initialized;
	int fd[fastmmapmq_bufferlength];
	volatile uint8_t *map[fastmmapmq_bufferlength+200];
	int currentcreatedmapindex;
	int indexb[fastmmapmq_bufferlength];
	uint32_t *futexpointers[fastmmapmq_bufferlength+200];
	int errdisplayed;
}fastmmapmq_instance;

//Global fastmmapmq state.
fastmmapmq_instance fastmmapmq_fastmmapinstance={.memmappedarraysize=0,.shmsize=0,.initialized=0};

void fastmmapmq_initfastmmapmq(){
	fastmmapmq_fastmmapinstance.memmappedarraysize=(fastmmapmq_bufferlength+100);
	fastmmapmq_fastmmapinstance.shmsize=((fastmmapmq_bufferlength+100)* sizeof(char));
	for(int i=0; i<fastmmapmq_bufferlength; i++)
		fastmmapmq_fastmmapinstance.fd[i]=-1;
	fastmmapmq_fastmmapinstance.currentcreatedmapindex=0;
	for(int i=0; i<fastmmapmq_bufferlength; i++)
		fastmmapmq_fastmmapinstance.indexb[i]=0;
	fastmmapmq_fastmmapinstance.errdisplayed=0;
	fastmmapmq_fastmmapinstance.initialized=1;
}
static int fastmmapmq_futex(uint32_t *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3){
	#ifdef fastmmapmq_futexavaliable
		return syscall(SYS_futex, uaddr, futex_op, val,timeout, uaddr, val3);
	#endif
}
int fastmmapmq_atomiccomparefastmmap(uint32_t *a, const uint32_t *b, int c){
#ifdef __cplusplus
	#pragma message "Using __sync_bool_compare_and_swap(a,*b,c) for atomic operation"
	return __sync_bool_compare_and_swap(a,*b,c);
#else
	#ifndef __clang__
		#pragma message "Using atomic_compare_exchange_strong(a,b,c) for atomic operation, in case of compile error change to __sync_bool_compare_and_swap(a,*b,c);"
		return atomic_compare_exchange_strong(a,b,c);//Note, this function uses pointer b, and __sync_bool_compare_and_swap uses the value (*b).
	#else
		#pragma message "Using __sync_bool_compare_and_swap(a,*b,c) for atomic operation"
		return __sync_bool_compare_and_swap(a,*b,c);
	#endif
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
			#ifndef fastmmapmq_futexavaliable
				if(fastmmapmq_fastmmapinstance.errdisplayed==0){
					perror("Warning, this version of FastMmapMQ is built without futex support, but a futex was required on mmap initialization.\nFalling back to spinlock, this may cause above than normal CPU usage");
					fastmmapmq_fastmmapinstance.errdisplayed=1;
				}
			#endif
			if(fastmmapmq_atomiccomparefastmmap(futexp, &one, 0)){
				return;
			}
			while(i<10){
				if(*futexp==0){
					break;
				}
				i++;
			}
			if(fastmmapmq_atomiccomparefastmmap(futexp, &one, 0)){
				return;
			}
			#ifdef fastmmapmq_futexavaliable
				if(fastmmapmq_atomiccomparefastmmap(futexp, &zero, 2)){
					s = fastmmapmq_futex(futexp, FUTEX_WAIT, 2, NULL, NULL, 0);// Wait to aquire the lock.
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
		if(fastmmapmq_atomiccomparefastmmap(futexp, &zero, 1)){
			return;	
		}
		#ifdef fastmmapmq_futexavaliable
		if(fastmmapmq_atomiccomparefastmmap(futexp, &two, 1)){
			s = fastmmapmq_futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);//Wake the other process. 
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
int fastmmapmq_search(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	int isearch=0;
	char temp[650]={'\0'};
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
int fastmmapmq_isinshm(char *fdlink,char *id){
	struct stat sb;
	char *linkname;
	ssize_t r;
	char *str;
	str=(char *)malloc(strlen(fastmmapmq_shmpath)+strlen(id)+3);
	strcpy(str, fastmmapmq_shmpath);
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
int fastmmapmq_openfd_connect(char *programlocation,char *id,mode_t permission){
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
				if(fastmmapmq_search(cmdlineuri,programlocation)){
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
						if(curfdnum>fastmmapmq_maxfdnum){
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
								if(fastmmapmq_isinshm(cmdlinefduric,id)){
										foundfile=1;
										char* location;
										location=(char *)malloc(strlen(fastmmapmq_shmpath)+strlen(id)+1+strlen(cmdlinefduric));
										strcpy(location, cmdlinefduric);
										fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex] = open(location, O_RDWR);
										if (fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex] == -1) {
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
	fastmmapmq_fastmmapinstance.currentcreatedmapindex=fastmmapmq_fastmmapinstance.currentcreatedmapindex+1;
	return fastmmapmq_fastmmapinstance.currentcreatedmapindex-1;
}
int fastmmapmq_openfd_create(char *programlocation,char *id,mode_t permission){
	srand(time(NULL));
		char strab[29]="";
		char *randomstring;
		randomstring=strab;
		sprintf(randomstring,"%i",(rand()));
		char* location;
		location=(char *)malloc(strlen(fastmmapmq_shmpath)+strlen(randomstring)+strlen(id)+5);
		strcpy(location, fastmmapmq_shmpath);
		strcat(location, id);
		strcat(location, "-");
		strcat(location, randomstring);
		fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex] = open(location, O_RDWR | O_CREAT, (mode_t) permission);
		if (fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex] == -1) {
			perror("Error opening shared memory");
			exit(EXIT_FAILURE);
		}
		char* locationbfgffsthf;
		locationbfgffsthf=(char *)malloc(strlen(fastmmapmq_shmpath)+strlen(randomstring)+1);
		strcpy(locationbfgffsthf, fastmmapmq_shmpath);
		strcat(locationbfgffsthf, randomstring);
		unlink(location);
	fastmmapmq_fastmmapinstance.currentcreatedmapindex=fastmmapmq_fastmmapinstance.currentcreatedmapindex+1;
	return fastmmapmq_fastmmapinstance.currentcreatedmapindex-1;
}
int fastmmapmq_initshm(void){
	int lseekresult;
	lseekresult = lseek(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1], 2*fastmmapmq_fastmmapinstance.shmsize+25+((fastmmapmq_sharedstringsize+3)*sizeof(char)), SEEK_SET);
	if (lseekresult == -1) {
		perror("lseek1");
		close(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]);
		return -1;
	}
	lseekresult = write(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1], "", 1);
	if (lseekresult != 1) {
		perror("lseek2");
		close(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]);
		return -1;
	}
	return 0;
}
int fastmmapmq_creatememmap(void){
	if(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]==-1){
		return -1;
	}
	fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1] = (volatile uint8_t *)mmap(0, fastmmapmq_fastmmapinstance.shmsize+((fastmmapmq_sharedstringsize+3)*sizeof(char)), PROT_READ | PROT_WRITE, MAP_SHARED, fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1], 0);
	if (fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1] == MAP_FAILED) {
		close(fastmmapmq_fastmmapinstance.fd[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]);
		perror("Error on shared memory mmap");
		return -1;
	}
	fastmmapmq_fastmmapinstance.futexpointers[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]= (uint32_t*)(&(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][4]));
	return 0;
}
int fastmmapmq_startmemmap(int create,char *programlocation,char *id, mode_t permission,int locking){
	if(fastmmapmq_fastmmapinstance.currentcreatedmapindex>fastmmapmq_bufferlength-5){
		perror("Maximum mmap number exceeded");
		exit(EXIT_FAILURE);
	}
	int thismapindex=-1;
	int openedshmstatus=1;
	if(create==1){
		thismapindex=fastmmapmq_openfd_create(programlocation,id,permission);
		openedshmstatus=fastmmapmq_initshm();
	}else{
		thismapindex=fastmmapmq_openfd_connect(programlocation,id,permission);
		openedshmstatus=0;
	}
	int jjold=0;
	if(thismapindex==-1){
		free(programlocation);
		return -1;
	}
	if(fastmmapmq_creatememmap()==-1){
		free(programlocation);
		return -1;
	}
	if(openedshmstatus==0){
		*(fastmmapmq_fastmmapinstance.futexpointers[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1])=1;//Start futex unlocked.
		char strab[9]="";
		fastmmapmq_fastmmapinstance.indexb[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]=0;
		if(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][16]!='\x17'){
		if(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][16]!='\x21'){
			uint32_t *indexaux1=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][0]));
			*indexaux1=0;
			uint32_t *indexaux2=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][8]));
			*indexaux2=0;
			uint32_t *indexaux3=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][12]));
			*indexaux3=0;
			if(locking==1){
				fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][fastmmapmq_fastmmapinstance.shmsize-42]='A';
			}else{
				fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][fastmmapmq_fastmmapinstance.shmsize-42]='B';
			}
			fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][16]='0';
		}
		}
		fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][16]='\x17';
		jjold=0;
		fastmmapmq_fastmmapinstance.indexb[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1]=0;
		jjold=0;
		while(jjold<=19){
			fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][fastmmapmq_fastmmapinstance.shmsize-(40-jjold)]=id[jjold];
			jjold=jjold+1;
			if(id[jjold-1]=='\0')
				break;
		}
		while(jjold<=19){
			fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][fastmmapmq_fastmmapinstance.shmsize-(40-jjold)]='\0';
			jjold=jjold+1;		
		}
		char* ididentfier=(char*)"luisvmffastmmapmq\x17\x17\x17";
		jjold=0;
		while(jjold<=19){
			fastmmapmq_fastmmapinstance.map[fastmmapmq_fastmmapinstance.currentcreatedmapindex-1][fastmmapmq_fastmmapinstance.shmsize-(20-jjold)]=ididentfier[jjold];
			jjold=jjold+1;
		}
		return thismapindex;
	}else{
		free(programlocation);
		return -1;
	}
}
void fastmmapmq_addresetcounter(int thismapindexreset){
	uint32_t *indexaux1=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[thismapindexreset][12]));
	*indexaux1=*indexaux1+1;
}
int fastmmapmq_getresetcounter(int thismapindexreset){
	uint32_t *indexaux1=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[thismapindexreset][12]));
	int aux=(int)*indexaux1;
	return aux;
}
int fastmmapmq_writemmap(int writemapindexselect,  char *s) {
	int jjold=0;
	char stra[9]="";
	char straold[9]="";
	if(writemapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(writemapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux;
	 lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[writemapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[writemapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[writemapindexselect]);
	jjold=0;
	uint32_t index=0;
	uint32_t indexreadold=0;
	int i=0;
	int lenscalc=strlen(s);
	uint32_t *indexaux1=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[writemapindexselect][0]));
	index=*indexaux1;
	uint32_t *indexaux2=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[writemapindexselect][8]));
	indexreadold=*indexaux2;
	int writeupto=index+lenscalc+1;
	if(fastmmapmq_fastmmapinstance.map[writemapindexselect][16]=='\x17'){
	if(index!=0){
		writeupto=writeupto-17;
		if(writeupto>fastmmapmq_bufferlength-1){
			writeupto=(writeupto-(fastmmapmq_bufferlength-1))-1;
		}
		if((writeupto-lenscalc-1)<indexreadold){
			if((writeupto)>=indexreadold){
				unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[writemapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[writemapindexselect]);
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
		if((i-1)<fastmmapmq_bufferlength+100)
		fastmmapmq_fastmmapinstance.map[writemapindexselect][i-1]=s[i-index];
		i=i+1;
		oldiwrite=i;
		if(i>fastmmapmq_bufferlength+17){
			resetwriteposf=1;
			oldiwrite=i;
			i=lenscalc+index+1;
			break;
		}
	}
	i=oldiwrite;
	if(resetwriteposf==1){
		fastmmapmq_addresetcounter(writemapindexselect);
		i=18;
		while(i<=lenscalc+18-(oldiwrite-index)){
			if((i-1)<fastmmapmq_bufferlength+100)
			fastmmapmq_fastmmapinstance.map[writemapindexselect][i-1]=s[oldiwrite+i-index-18];
			i=i+1;
		}
	}
	if((i-2)<fastmmapmq_bufferlength+100)
	fastmmapmq_fastmmapinstance.map[writemapindexselect][i-2]=' ';
	index=i-1;
	uint32_t *pointerindex=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[writemapindexselect][0]));
	*pointerindex=index;
	if(index>=fastmmapmq_bufferlength+18){
		*pointerindex=0;
		fastmmapmq_addresetcounter(writemapindexselect);
	}
	unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[writemapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[writemapindexselect]);
   return 0;
}
char *fastmmapmq_readmmap(int readmapindexselect,int gfifghdughfid) {
	int jjold=0;
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux;
	lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[readmapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	if(gfifghdughfid==0){
		char *stra=(char *)malloc((fastmmapmq_maxmemreturnsize+100)*sizeof(char));
		if(stra==NULL){
			perror("Malloc fail on readmmap");
			exit(EXIT_FAILURE);
		}
		int n=0;
		while(n<(fastmmapmq_maxmemreturnsize+100)){
			stra[n]='\0';
			n++;
		}
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		uint32_t index=0;
		jjold=0;
		lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		uint32_t *indexauxaa=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[readmapindexselect][0]));
		index=*indexauxaa;
		fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=0;
		jjold=0;
		uint32_t *indexauxbb=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[readmapindexselect][8]));
		fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=(int)*indexauxbb;
		unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		int i=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+17;
		if(fastmmapmq_fastmmapinstance.indexb[readmapindexselect]==index-17){
			tmpstr[0]='\0';
			return tmpstr;
		}else{
		if(fastmmapmq_fastmmapinstance.indexb[readmapindexselect]==index){
			if(index==0){
				tmpstr[0]='\0';
				return tmpstr;
			}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<fastmmapmq_bufferlength+17){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				if((i)<fastmmapmq_bufferlength+100){
				if(fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=='\0'){
					tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17;
			fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=0;
			i=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+17;
			while(i<index){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[offsetretarray+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				if((i)<fastmmapmq_bufferlength+100){
				if(fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
		}
		fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17;
		lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		uint32_t *pointerindex=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[readmapindexselect][8]));
		*pointerindex=fastmmapmq_fastmmapinstance.indexb[readmapindexselect];
		unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		return tmpstr;
		}
	}else{
		fastmmapmq_fastmmapinstance.map[readmapindexselect][16]='\x21';
		lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		char *stra=(char *)malloc((fastmmapmq_maxmemreturnsize+100)*sizeof(char));
		if(stra==NULL){
			perror("Malloc fail on readmmap");
			exit(EXIT_FAILURE);
		}
		int n=0;
		while(n<(fastmmapmq_maxmemreturnsize+100)){
			stra[n]='\0';
			n++;
		}
		char *tmpstr;
		tmpstr=stra;
		char strab[9]="";
		int index=0;
		uint32_t *pointerindex=(uint32_t*)(&(fastmmapmq_fastmmapinstance.map[readmapindexselect][0]));
		index=(int)*pointerindex;
		unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
		int i=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+17;
		if(fastmmapmq_fastmmapinstance.indexb[readmapindexselect]==index-17){
			tmpstr[0]='\0';
			return tmpstr;
		}else{
		if(fastmmapmq_fastmmapinstance.indexb[readmapindexselect]==index){
			if(index==0){
				tmpstr[0]='\0';
				return tmpstr;
			}
		}
		if(i>=index){
			int offsetretarray=0;
			while(i<fastmmapmq_bufferlength+17){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				if((i)<fastmmapmq_bufferlength+100){
				if(fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=='\0'){
					tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
			offsetretarray=i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17;
			fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=0;
			i=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+17;
			while(i<index){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[offsetretarray+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				if((i)<fastmmapmq_bufferlength+100){
				if(fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=='\0'){
					tmpstr[offsetretarray+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=' ';
				}}
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
		}else{
			while(i<index){
				if((i)<fastmmapmq_bufferlength+100)
				tmpstr[i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
				i=i+1;
				if(i>=fastmmapmq_maxmemreturnsize+17){
					break;
				}
			}
		}
		fastmmapmq_fastmmapinstance.indexb[readmapindexselect]=fastmmapmq_fastmmapinstance.indexb[readmapindexselect]+i-fastmmapmq_fastmmapinstance.indexb[readmapindexselect]-17;
		return tmpstr;
		}
	}
}
int fastmmapmq_connectmmap(char *b,char *s) {
	if(fastmmapmq_fastmmapinstance.initialized==0){
		fastmmapmq_initfastmmapmq();
	}
	char *prog;
	prog=(char *)malloc(strlen(b)+1);
	sprintf(prog,"%s",b);
	mode_t permission=(mode_t)0000;
	return fastmmapmq_startmemmap(0,prog,s,permission,1);
}
int fastmmapmq_createmmap(char *b,char *s,int locking) {
	if(fastmmapmq_fastmmapinstance.initialized==0){
		fastmmapmq_initfastmmapmq();
	}
	char *prog;
	prog=(char *)malloc(strlen(b)+1);
	char *n=(char*)"None";
	sprintf(prog,"%s",n);
	char *perm=s;
	mode_t permission=(((perm[0]=='r')*4|(perm[1]=='w')*2|(perm[2]=='x'))<<6)|(((perm[3]=='r')*4|(perm[4]=='w')*2|(perm[5]=='x'))<<3)|(((perm[6]=='r')*4|(perm[7]=='w')*2|(perm[8]=='x')));
	int iaux=fastmmapmq_startmemmap(1,prog,b,permission,locking);
	return iaux;
}
char *fastmmapmq_getsharedstring(int readmapindexselect) {
	char *tmpstring=(char *)malloc(fastmmapmq_sharedstringsize+5);
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[readmapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	int i=fastmmapmq_fastmmapinstance.memmappedarraysize;
	while(i<fastmmapmq_fastmmapinstance.memmappedarraysize+fastmmapmq_sharedstringsize){
		tmpstring[i-fastmmapmq_fastmmapinstance.memmappedarraysize]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
		if(fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=='\0'){
			break;
		}
		i=i+1;
	}
	unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	return tmpstring;
}
int fastmmapmq_writesharedstring(int readmapindexselect,char* tmpstring) {
	char stra[fastmmapmq_sharedstringsize+5]="";
	if(readmapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[readmapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	int i=fastmmapmq_fastmmapinstance.memmappedarraysize;
	while(i<fastmmapmq_fastmmapinstance.memmappedarraysize+fastmmapmq_sharedstringsize){
		fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=tmpstring[i-fastmmapmq_fastmmapinstance.memmappedarraysize];
		i=i+1;
		if(tmpstring[i-fastmmapmq_fastmmapinstance.memmappedarraysize-1]=='\0'){
			break;
		}
	}
	unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	return 0;
}


int fastmmapmq_writesharedstring_withsize(int readmapindexselect,char* tmpstring,int sharedstrlenpriv) {
	char stra[fastmmapmq_sharedstringsize+5]="";
	if(readmapindexselect<0){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on write");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[readmapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	int i=fastmmapmq_fastmmapinstance.memmappedarraysize;
	while(i<fastmmapmq_fastmmapinstance.memmappedarraysize+fastmmapmq_sharedstringsize){
		fastmmapmq_fastmmapinstance.map[readmapindexselect][i]=tmpstring[i-fastmmapmq_fastmmapinstance.memmappedarraysize];
		i=i+1;
		if(i-fastmmapmq_fastmmapinstance.memmappedarraysize-1>=sharedstrlenpriv){
			break;
		}
	}
	unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	return 0;
}

char *fastmmapmq_getsharedstring_withsize(int readmapindexselect,int sharedstrlenpriv) {
	char *tmpstring=(char *)malloc(fastmmapmq_sharedstringsize+5);
	if(readmapindexselect<0){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	if(readmapindexselect>fastmmapmq_fastmmapinstance.currentcreatedmapindex){
		perror("Invalid mmap id on read");
		exit(EXIT_FAILURE);
	}
	uint32_t lockingaux=0;
	if(fastmmapmq_fastmmapinstance.map[readmapindexselect][fastmmapmq_fastmmapinstance.shmsize-42]=='A'){
		lockingaux=1;
	}
	lockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	int i=fastmmapmq_fastmmapinstance.memmappedarraysize;
	while(i<fastmmapmq_fastmmapinstance.memmappedarraysize+fastmmapmq_sharedstringsize){
		tmpstring[i-fastmmapmq_fastmmapinstance.memmappedarraysize]=fastmmapmq_fastmmapinstance.map[readmapindexselect][i];
		i=i+1;
		if(i-fastmmapmq_fastmmapinstance.memmappedarraysize-1>=sharedstrlenpriv){
			break;
		}
	}
	unlockfastmmapmq(fastmmapmq_fastmmapinstance.futexpointers[readmapindexselect],lockingaux,fastmmapmq_fastmmapinstance.fd[readmapindexselect]);
	return tmpstring;
}
