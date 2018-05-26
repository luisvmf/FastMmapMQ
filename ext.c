#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
//Copyright (c) 2018 Luís Victor Muller Fabris. Apache License.
//TODO:Add buffer overrun warning.
//---------------------------------------------------------
//---------------------------------------------------------
//Constants
#define shmpath "/dev/shm/fastmmapmqshm-"
#define bufferlength  (999999) //Number of characters in buffer. Maximum value is 999999.
#define maxmemreturnsize  (999999) //Maximum number of characters returned by read function in one run. Maximum value is 999999.
//---------------------------------------------------------
//---------------------------------------------------------
#define memmappedarraysize  (bufferlength+100)
#define shmsize (memmappedarraysize * sizeof(char))
int fd;
char *map;
int readpos;
void openfd(char *id){
	char* location;
	location=malloc(strlen(shmpath)+strlen(id)+1);
	strcpy(location, shmpath);
	strcat(location, id);
	fd = open(location, O_RDWR | O_CREAT, (mode_t)0600);
	if (fd == -1) {
		perror("Error opening shared memory");
		exit(EXIT_FAILURE);
	}
}
void initshm(){
	int lseekresult;
	lseekresult = lseek(fd, shmsize-1, SEEK_SET);
	if (lseekresult == -1) {
		close(fd);
		perror("Error on lseek");
		exit(EXIT_FAILURE);
	}
	lseekresult = write(fd, "", 1);
	if (lseekresult != 1) {
		close(fd);
		perror("Error on shared memory initialization");
		exit(EXIT_FAILURE);
	}
}
void creatememmap(){
	map = mmap(0, shmsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		perror("Error on shared memory mmap");
		exit(EXIT_FAILURE);
	}
}
int startmemmap(char *id){
	openfd(id);
	initshm();
	creatememmap();
	if(map[7]!='\x17'){
		map[0]='0';
		map[1]='0';
		map[2]='0';
		map[3]='0';
		map[4]='0';
		map[5]='0';
		map[6]='0';
		map[8]='0';
		map[9]='0';
		map[10]='0';
		map[11]='0';
		map[12]='0';
		map[13]='0';
		map[14]='0';
		map[15]='0';
		map[16]='0';
	}
	map[7]='\x17';
	//map[17]='\x18';
	return 0;
}
static PyObject* writemessage(PyObject* self,  PyObject *args) {
	char *s;
	char stra[9]="";
	char *datapos;
	datapos=stra;
	if (!PyArg_ParseTuple(args, "s", &s)) {
		return NULL;
	}
	datapos[0]=map[0];
	datapos[1]=map[1];
	datapos[2]=map[2];
	datapos[3]=map[3];
	datapos[4]=map[4];
	datapos[5]=map[5];
	datapos[6]=map[6];
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
		map[i-1]=s[i-index];
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
		i=18;
		while(i<=lenscalc+18-(oldiwrite-index)){
			map[i-1]=s[oldiwrite+i-index-18];
			i=i+1;
		}
	}
	map[i-2]=' ';
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
	map[0]=vca+'0';
	map[1]=vcb+'0';
	map[2]=vcc+'0';
	map[3]=vcd+'0';
	map[4]=vce+'0';
	map[5]=vcf+'0';
	map[6]=vcg+'0';
	if(index>=bufferlength+18){
		map[0]='0';
		map[1]='0';
		map[2]='0';
		map[3]='0';
		map[4]='0';
		map[5]='0';
		map[6]='0';
		char *dataposbc;
		dataposbc=stra;
		int indexc=0;
		sprintf(dataposbc,"%c%c",map[15],map[16]);
		indexc=atoi(dataposbc);
		indexc=indexc+1;
		int vindexca=indexc/10;
		map[15]=vindexca+'0';
		map[16]=(indexc-vindexca*10)+'0';
	}
   return Py_BuildValue("i", 0);
}
static PyObject* readmessage(PyObject* self) {
	char stra[maxmemreturnsize+100]="";
	char *tmpstr;
	tmpstr=stra;
	char strab[9]="";
	char *datapos;
	datapos=strab;
	int index=0;
	datapos[0]=map[0];
	datapos[1]=map[1];
	datapos[2]=map[2];
	datapos[3]=map[3];
	datapos[4]=map[4];
	datapos[5]=map[5];
	datapos[6]=map[6];
	index=atoi(datapos);
	char *dataposb;
	dataposb=strab;
	int indexb=0;
	dataposb[0]=map[8];
	dataposb[1]=map[9];
	dataposb[2]=map[10];
	dataposb[3]=map[11];
	dataposb[4]=map[12];
	dataposb[5]=map[13];
	dataposb[6]=map[14];
	indexb=atoi(dataposb);
	char *dataposbc;
	dataposbc=strab;
	int indexc=0;
	sprintf(dataposbc,"%c%c",map[15],map[16]);
	indexc=atoi(dataposbc);
	int i=indexb+17;
	if(indexb==index-17){
		return Py_BuildValue("s","");
	}else{
	if(indexb==index){
	if(index==0){
		return Py_BuildValue("s","");
	}
	}
	if(i>=index){
		int offsetretarray=0;
		while(i<bufferlength+17){
			tmpstr[i-indexb-17]=map[i];
			if(map[i]=='\0'){
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
			tmpstr[offsetretarray+i-indexb-17]=map[i];
			if(map[i]=='\0'){
				tmpstr[offsetretarray+i-indexb-17]=' ';
			}
			i=i+1;
			if(i>=maxmemreturnsize+17){
				break;
			}
		}
	}else{
		while(i<index){
			tmpstr[i-indexb-17]=map[i];
			i=i+1;
			if(i>=maxmemreturnsize+17){
				break;
			}
		}
	}
	indexb=indexb+i-indexb-17;
	int vca=indexb/1000000;
	int vcb=(indexb-vca*1000000)/100000;
	int vcc=(indexb-vca*1000000-vcb*100000)/10000;
	int vcd=(indexb-vca*1000000-vcb*100000-vcc*10000)/1000;
	int vce=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000)/100;
	int vcf=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100)/10;
	int vcg=(indexb-vca*1000000-vcb*100000-vcc*10000-vcd*1000-vce*100-vcf*10);
	map[8]=vca+'0';
	map[9]=vcb+'0';
	map[10]=vcc+'0';
	map[11]=vcd+'0';
	map[12]=vce+'0';
	map[13]=vcf+'0';
	map[14]=vcg+'0';
	return Py_BuildValue("s",tmpstr );
	}
}
static PyObject* pyinitmmap(PyObject* self,  PyObject *args) {
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s)) {
		return NULL;
	}
	return Py_BuildValue("i", startmemmap(s));
}
static char mmap_docs[] =
   "write('data'): Write data\nread(): Read data\nconnectmap('id'): Connects to write/read messages from this id. The id can contain any characters valid in a file name (no / or null character).This function must be called before write() or read()\n";

static PyMethodDef mmap_funcs[] = {
	{"write", (PyCFunction)writemessage, METH_VARARGS, mmap_docs},
	{"read", (PyCFunction)readmessage, METH_NOARGS, mmap_docs},
	{"connectmap", (PyCFunction)pyinitmmap, METH_VARARGS, mmap_docs},
	{ NULL, NULL, 0, NULL}
};
void initfastmmap(void) {
	Py_InitModule3("fastmmap", mmap_funcs,"Fast IPC with mmap");
}
