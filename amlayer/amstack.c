#include <stdio.h>
#include "am.h"
#include "pf.h"

#define AM_MAXSTACK 50

static struct {
     int pageNumber;
     int offset;
} AM_Stack[AM_MAXSTACK];

static int AM_topofStackPtr = -1;

int AM_PushStack(int pageNum, int offset)
{
    AM_topofStackPtr++;
    AM_Stack[AM_topofStackPtr].pageNumber = pageNum;
    AM_Stack[AM_topofStackPtr].offset = offset;
    return 0;
}

int AM_PopStack(void)
{
    AM_topofStackPtr--;
    return 0;
}

int AM_topofStack(int *pageNum, int *offset)
{
    *pageNum = AM_Stack[AM_topofStackPtr].pageNumber;
    *offset = AM_Stack[AM_topofStackPtr].offset;
    return 0;
}

int AM_EmptyStack(void)
{
    AM_topofStackPtr = -1;
    return 0;
}
