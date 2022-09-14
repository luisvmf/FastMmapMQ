//---------------------------------------------------------
//---------------------------------------------------------
//Copyright (c) 2022 Luís Victor Muller Fabris. Apache License.
//---------------------------------------------------------
//---------------------------------------------------------

#ifndef FASTMMAPMQ_H_
	#define FASTMMAPMQ_H_
	int fastmmapmq_writemmap(int writemapindexselect,  char *s);
	char *fastmmapmq_readmmap(int readmapindexselect,int gfifghdughfid);
	int fastmmapmq_connectmmap(char *b,char *s);
	int fastmmapmq_createmmap(char *b,char *s,int locking);
	char *fastmmapmq_getsharedstring(int readmapindexselect);
	int fastmmapmq_writesharedstring(int readmapindexselect,char* tmpstring);
#endif
