#include "src/fastmmapmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//TODO check program hang without sleep

void main(void){
	printf("Connecting...\n");
	int mapid=fastmmapmq_connectmmap("receive_example","sendtestmmapid");
	while(mapid==-1){
		mapid=fastmmapmq_connectmmap("receive_example","sendtestmmapid");
		sleep(0.1);
	}
	printf("Connected, sending message...\n");
	fastmmapmq_writemmap(mapid,"Hello from ./send_example !!!");
	printf("Message sent, check the other proccess!\n");
	printf("Terminating...\n");
}
