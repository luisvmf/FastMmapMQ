#include "cmodule.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void main(){
int mapid=connectmmap("None","testmmapidb");
int mapidb=connectmmap("test","testmmapidb");
writemmap(mapid,"aaab");
writemmap(mapid,"aaab");
printf("%s",readmmap(mapidb,1));
}
