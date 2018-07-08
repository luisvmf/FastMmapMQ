#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(void){
int mapid=connectmmap("None","testmmapidb");
int mapidhjkc=connectmmap("None","testmmapbe");
int mapidb=connectmmap("test","testmmapidb");
char *foundmaps[20];
listmmaps("test",foundmaps,9);
printf("%s\n%s\n%s\n",foundmaps[0],foundmaps[1],foundmaps[2]);
writemmap(mapid,"aaab");
writemmap(mapid,"aaab");
printf("%s",readmmap(mapidb,1));
}
