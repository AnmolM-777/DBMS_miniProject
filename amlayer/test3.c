#include <stdio.h>
#include <stdlib.h>
#include "am.h"
#include "pf.h"
#include "testam.h"

#define MAXRECS	512
#define FNAME_LENGTH 80

int main(void)
{
int fd;
char fname[FNAME_LENGTH];
int recnum;
int sd;
int numrec;
int testval;	

	printf("initializing\n");
	PF_Init();

	printf("creating index\n");
	AM_CreateIndex(RELNAME, 0, INT_TYPE, sizeof(int));

	printf("opening index\n");
	snprintf(fname, sizeof(fname), "%s.0", RELNAME);
	fd = PF_OpenFile(fname);

	printf("inserting into index\n");
	for (recnum=0; recnum < 20; recnum++){
		AM_InsertEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}
	printf("deleting odd number records\n");
	for (recnum=1; recnum < 20; recnum += 2)
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));

	printf("retrieving even number records\n");
	numrec= 0;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), EQ_OP, NULL);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);
	AM_CloseIndexScan(sd);

	printf("deleting even number records\n");
	for (recnum=0; recnum < 20; recnum += 2)
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));

	printf("retrieving from empty index\n");
	numrec= 0;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), EQ_OP, NULL);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);
	AM_CloseIndexScan(sd);

	printf("begin test of complex delete\n");
	printf("inserting into index\n");
	for (recnum=0; recnum < MAXRECS; recnum+=2){
		AM_InsertEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}
	for (recnum=1; recnum < MAXRECS; recnum+=2)
		AM_InsertEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));

	printf("deleting everything\n");
	for (recnum=1; recnum < MAXRECS; recnum += 2)
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	for (recnum=0; recnum < MAXRECS; recnum +=2)
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));

	printf("printing empty index\n");
	numrec= 0;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), EQ_OP, NULL);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);
	AM_CloseIndexScan(sd);

	printf("inserting everything back\n");
	for (recnum=0; recnum < MAXRECS; recnum++){
		AM_InsertEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}

	printf("delete records less than 100\n");
	testval = 100;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), LT_OP, (char *)&testval);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		if (recnum >= 100){
			printf("invalid recnum %d\n", recnum);
			exit(1);
		}
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}
	AM_CloseIndexScan(sd);

	printf("delete records greater than 150\n");
	testval = 150;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), GT_OP, (char *)&testval);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		if (recnum <= 150){
			printf("invalid recnum %d\n", recnum);
			exit(1);
		}
		AM_DeleteEntry(fd, INT_TYPE, sizeof(int), (char *)&recnum, IntToRecId(recnum));
	}
	AM_CloseIndexScan(sd);

	printf("printing between 100 and 150\n");
	numrec= 0;
	sd = AM_OpenIndexScan(fd, INT_TYPE, sizeof(int), EQ_OP, NULL);
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);
	AM_CloseIndexScan(sd);

	printf("closing down\n");
	PF_CloseFile(fd);
	AM_DestroyIndex(RELNAME, 0);

	printf("test3 done!\n");
	return 0;
}
