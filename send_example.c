#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(void){
printf("Connecting...\n");
int mapid=connectmmap("receive_example","sendtestmmapid");
while(mapid==-1){
	mapid=connectmmap("receive_example","sendtestmmapid");
	sleep(0.1);
}
printf("Connected, sending message...\n");
writemmap(mapid,"Hello from ./send_example !!!");
printf("Message sent, check the other proccess!\n");
printf("Terminating...\n");
}
