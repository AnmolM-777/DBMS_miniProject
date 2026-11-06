#include <stdio.h>
#include "am.h"
#include "pf.h"
#include "testam.h"

#define MAXRECS	50
#define MAX_FNAME_LENGTH 80

int main(void)
{
int id0, id1;
char ch;
int sd0, sd1;
char buf[NAMELENGTH];
char fnamebuf[MAX_FNAME_LENGTH];
int recnum;
int numrec;

	printf("initializing\n");
	PF_Init();

	printf("creating indices\n");
	AM_CreateIndex(RELNAME, RECNAME_INDEXNO, CHAR_TYPE, NAMELENGTH);
	AM_CreateIndex(RELNAME, RECVAL_INDEXNO, INT_TYPE, sizeof(int));

	printf("opening indices\n");
	snprintf(fnamebuf, sizeof(fnamebuf), "%s.%d", RELNAME, RECNAME_INDEXNO);
	id0 = PF_OpenFile(fnamebuf);
	snprintf(fnamebuf, sizeof(fnamebuf), "%s.%d", RELNAME, RECVAL_INDEXNO);
	id1 = PF_OpenFile(fnamebuf);

	printf("inserting into index on char\n");
	for(ch='a', recnum=0; ch <= 'z'; ch= succ(ch), recnum++){
		snprintf(buf, sizeof(buf), "%c%d", ch, recnum);
		AM_InsertEntry(id0, CHAR_TYPE, NAMELENGTH, buf, IntToRecId(recnum));
	}

	printf("opening index scan on char\n");
	sd0 = AM_OpenIndexScan(id0, CHAR_TYPE, NAMELENGTH, EQ_OP, NULL);
	printf("retrieving recid's from scan descriptor %d\n", sd0);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd0)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);

	printf("inserting into index of integer\n");
	for (recnum=0; recnum < MAXRECS; recnum++){
		AM_InsertEntry(id1, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}

	printf("opening index scan on integer\n");
	sd1 = AM_OpenIndexScan(id1, INT_TYPE, sizeof(int), EQ_OP, NULL);
	printf("retrieving recid's from scan descriptor %d\n", sd1);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd1)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);

	printf("closing down\n");
	AM_CloseIndexScan(sd0);
	AM_CloseIndexScan(sd1);
	PF_CloseFile(id0);
	PF_CloseFile(id1);
	AM_DestroyIndex(RELNAME, RECNAME_INDEXNO);
	AM_DestroyIndex(RELNAME, RECVAL_INDEXNO);

	printf("test1 done!\n");
	return 0;
}
