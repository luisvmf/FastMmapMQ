#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(void){
int mapid=createmmap("sendtestmmapid","rwx------",1);


char *receive;
printf("Waiting for message from ./send_example...\n");
receive=readmmap(mapid,1);
while(receive[0]=='\0'){
	free(receive);
	receive=readmmap(mapid,1);
	sleep(0.1);
}
printf("Received message from ./send_example: %s\n",receive);
free(receive);
}
