#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int main(void){
	int mapid=connectmmap("test","testmmapidb");
	while(mapid==-1){
		mapid=connectmmap("test","testmmapidb");
		sleep(0.1);
	}

	//int i=9000000000;
	int j=0;
	while(1){
		//writemmap(mapid,"aaab");
		char *a=readmmap(mapid,1);
	int i=0;
    for(i=0; a[i] != '\0'; i++){}
	//printf("%s\n",a);
	j=j+i;
	free(a);
	usleep(300);
		//printf("%s\n",a);
	//printf("%i\n",j);
	//printf("%i\n",j);
	printf("%i\n",j);
	}
}
