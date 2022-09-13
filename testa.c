#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int main(void){
	int mapid=createmmap("testmmapidb","rwx------");

	int i=900000;
	while(i>0){
		writemmap(mapid,"aaab");
		//char *a=readmmap(mapid,1);
		i--;
	}
	//usleep(50000000);
}
