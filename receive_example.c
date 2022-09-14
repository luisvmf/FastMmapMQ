#include "src/fastmmapmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//TODO check program hang without sleep

void main(void){
	int mapid=fastmmapmq_createmmap("sendtestmmapid","rwx------",1);
	char *receive;
	printf("Waiting for message from ./send_example...\n");
	receive=fastmmapmq_readmmap(mapid,1);
	while(receive[0]=='\0'){
		free(receive);
		receive=fastmmapmq_readmmap(mapid,1);
		sleep(0.1);
	}
	printf("Received message from ./send_example: %s\n",receive);
	free(receive);
}
