#include <stdio.h>
#include <string.h>
#include "am.h"
#include "pf.h"
#include "testam.h"

#define STRING_SIZE 250
#define MAXRECS	100
#define FNAME_LENGTH 80

static void padstring(char *str, int length)
{
	int i;
	for (i = strlen(str) + 1, str = str + i; i < length; i++, str++)
		*str = '\0';
}

int main(void)
{
int fd;
char fname[FNAME_LENGTH];
char buf[STRING_SIZE];
int recnum;
int sd;
int numrec;

	printf("initializing\n");
	PF_Init();

	printf("creating index\n");
	AM_CreateIndex(RELNAME, 0, CHAR_TYPE, STRING_SIZE);

	printf("opening index\n");
	snprintf(fname, sizeof(fname), "%s.0", RELNAME);
	fd = PF_OpenFile(fname);

	printf("inserting into index on char\n");
	for (recnum=0; recnum < MAXRECS; recnum++){
		snprintf(buf, sizeof(buf), "%d", recnum);
		padstring(buf, STRING_SIZE);
		AM_InsertEntry(fd, CHAR_TYPE, STRING_SIZE, buf, IntToRecId(recnum));
	}

	printf("opening index scan on char\n");
	sd = AM_OpenIndexScan(fd, CHAR_TYPE, STRING_SIZE, EQ_OP, NULL);
	printf("retrieving recid's from scan descriptor %d\n", sd);
	numrec = 0;
	while((recnum=RecIdToInt(AM_FindNextEntry(sd)))>= 0){
		printf("%d\n", recnum);
		numrec++;
	}
	printf("retrieved %d records\n", numrec);

	printf("closing down\n");
	AM_CloseIndexScan(sd);
	PF_CloseFile(fd);
	AM_DestroyIndex(RELNAME, 0);

	printf("test2 done!\n");
	return 0;
}
