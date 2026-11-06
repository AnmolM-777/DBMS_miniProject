#ifndef TESTAM_H
#define TESTAM_H

typedef int RecIdType;

#define RecIdToInt(recid)	(recid)
#define IntToRecId(intval)	(intval)

#define CHAR_TYPE  'c'
#define INT_TYPE   'i'
#define FLOAT_TYPE 'f'
#define INT_SIZE   sizeof(int)
#define FLOAT_SIZE sizeof(float)

#define EQ_OP	1
#define LT_OP	2
#define GT_OP	3
#define LE_OP   4
#define GE_OP	5
#define NE_OP	6

#define RELNAME "testrel"

#define NAMELENGTH 11
#define RECNAME_INDEXNO 0
#define RECVAL_INDEXNO 1

typedef struct smallrec {
	char recname[NAMELENGTH];
	int recval;
} smallrec;

#define succ(ch) ((char)((int)(ch)+1))

#endif
