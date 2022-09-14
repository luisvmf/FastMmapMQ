#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(void){
int mapid=createmmap("testmmapidb","rwx------",1);
int mapidhjkc=createmmap("testmmapbe","rwx------",1);
int mapidb=connectmmap("test","testmmapidb");
char *foundmaps[20];
writemmap(mapid,"aaab");
writemmap(mapid,"aaab");
printf("%s\n",readmmap(mapidb,1));
writemmap(mapid,"aaac");
printf("%s\n",readmmap(mapidb,1));
writesharedstring(mapid,"shared string");
printf("%s\n",getsharedstring(mapidb));
}
