#include <stdio.h>
#include <string.h>
#include "am.h"
#include "pf.h"

int AM_Search(int fileDesc, char attrType, int attrLength, char *value, int *pageNum, char **pageBuf, int *indexPtr)
{
	int errVal;
	int nextPage;
	AM_LEAFHEADER lhead, *lheader;
	AM_INTHEADER ihead, *iheader;

	lheader = &lhead;
	iheader = &ihead;

	errVal = PF_GetFirstPage(fileDesc, pageNum, pageBuf);
	AM_Check;

	if (**pageBuf == 'l') 
	{
		memcpy(lheader, *pageBuf, AM_sl);
		if (lheader->attrLength != attrLength)
			return(AME_INVALIDATTRLENGTH);
	}
	else 
	{
		memcpy(iheader, *pageBuf, AM_sint);
		if (iheader->attrLength != attrLength)
			return(AME_INVALIDATTRLENGTH);
	}

	while ((**pageBuf) != 'l')
	{
		nextPage = AM_BinSearch(*pageBuf, attrType, attrLength, value, indexPtr, iheader);

		AM_PushStack(*pageNum, *indexPtr);

		errVal = PF_UnfixPage(fileDesc, *pageNum, FALSE);
		AM_Check;

		*pageNum = nextPage;

		errVal = PF_GetThisPage(fileDesc, *pageNum, pageBuf);
		AM_Check;

		if (**pageBuf == 'l') 
		{
			memcpy(lheader, *pageBuf, AM_sl);
			if (lheader->attrLength != attrLength)
				return(AME_INVALIDATTRLENGTH);
		}
		else
		{
			memcpy(iheader, *pageBuf, AM_sint);
			if (iheader->attrLength != attrLength)
				return(AME_INVALIDATTRLENGTH);
		}
	}

	return(AM_SearchLeaf(*pageBuf, attrType, attrLength, value, indexPtr, lheader));
}

int AM_BinSearch(char *pageBuf, char attrType, int attrLength, char *value, int *indexPtr, AM_INTHEADER *header)
{
    int i, childPage, recSize = sizeof(int) + attrLength;
    /* Internal layout: child0, (separator key, child) repeated. A separator
       is the smallest key in its right subtree, so equality goes right. */
    memcpy(&childPage, pageBuf + AM_sint, sizeof(int));
    *indexPtr = 0;
    for (i = 0; i < header->numKeys; ++i) {
        char *keyPtr = pageBuf + AM_sint + sizeof(int) + i * recSize;
        int cmp = AM_Compare(keyPtr, attrType, attrLength, value);
        if (cmp < 0) break; /* value is smaller: retain current child */
        memcpy(&childPage, keyPtr + attrLength, sizeof(int));
        *indexPtr = i + 1;
    }
    return childPage;
}

int AM_SearchLeaf(char *pageBuf, char attrType, int attrLength, char *value, int *indexPtr, AM_LEAFHEADER *header)
{
    int i, recSize = attrLength + AM_ss;
    for (i = 0; i < header->numKeys; ++i) {
        char *keyPtr = pageBuf + AM_sl + i * recSize;
        int cmp = AM_Compare(keyPtr, attrType, attrLength, value);
        if (cmp == 0) { *indexPtr = i + 1; return AM_FOUND; }
        if (cmp < 0) { *indexPtr = i + 1; return AM_NOT_FOUND; }
    }
    *indexPtr = header->numKeys + 1;
    return AM_NOT_FOUND;
}

int AM_Compare(char *bufPtr, char attrType, int attrLength, char *valPtr)
{
	int bufint, valint;
	float buffloat, valfloat;

	switch(attrType)
	{
	case 'i': 
		{
			memcpy((char *)&bufint, bufPtr, AM_si);
			memcpy((char *)&valint, valPtr, AM_si);
			if (valint < bufint) return(-1);
			else if (valint > bufint) return(1);
			else return(0);
		}
	case 'f': 
		{
			memcpy((char *)&buffloat, bufPtr, AM_sf);
			memcpy((char *)&valfloat, valPtr, AM_sf);
			if (valfloat < buffloat) return(-1);
			else if (valfloat > buffloat) return(1);
			else return(0);
		}
	case 'c': 
		{
			return(strncmp(valPtr, bufPtr, attrLength));
		}
	}
	return 0;
}
