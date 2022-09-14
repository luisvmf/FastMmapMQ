#include "src/fastmmapmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(void){
int mapid=fastmmapmq_createmmap("testmmapidb","rwx------",1);
int mapidhjkc=fastmmapmq_createmmap("testmmapbe","rwx------",1);
int mapidb=fastmmapmq_connectmmap("test","testmmapidb");
char *foundmaps[20];
fastmmapmq_writemmap(mapid,"aaab");
fastmmapmq_writemmap(mapid,"aaab");
printf("%s\n",fastmmapmq_readmmap(mapidb,1));
fastmmapmq_writemmap(mapid,"aaac");
printf("%s\n",fastmmapmq_readmmap(mapidb,1));
fastmmapmq_writesharedstring(mapid,"shared string");
printf("%s\n",fastmmapmq_getsharedstring(mapidb));
}
