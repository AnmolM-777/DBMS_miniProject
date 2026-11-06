#include <stdio.h>
#include <string.h>
#include "am.h"
#include "pf.h"

int AM_InsertintoLeaf(char *pageBuf, int attrLength, char *value, int recId, int index, int status)
{
	int recSize;
	char tempPage[PF_PAGE_SIZE];
	AM_LEAFHEADER head, *header;

	header = &head;
	memcpy(header, pageBuf, AM_sl);

	recSize = attrLength + AM_ss;
	if (status == AM_FOUND)
	{
		if (header->freeListPtr == 0)
			if ((header->recIdPtr - header->keyPtr) < (AM_si + AM_ss))
			{
				return(FALSE);
			}
		AM_InsertToLeafFound(pageBuf, recId, index, header);
		memcpy(pageBuf, header, AM_sl);
		return(TRUE);
	}

	if ((header->freeListPtr) == 0)
		if ((header->recIdPtr - header->keyPtr) < (AM_si + AM_ss + recSize))
			return(FALSE);
		else
		{    
			AM_InsertToLeafNotFound(pageBuf, value, recId, index, header);
			header->numKeys++;
			memcpy(pageBuf, header, AM_sl);
			return(TRUE);
		}
	else if ((header->recIdPtr - header->keyPtr) > recSize)
	{
		AM_InsertToLeafNotFound(pageBuf, value, recId, index, header);
		header->numKeys++;
		memcpy(pageBuf, header, AM_sl);
		return(TRUE);
	}
	else if (((header->numinfreeList)*(AM_si + AM_ss) + header->recIdPtr - header->keyPtr) > (recSize + AM_si + AM_ss))
	{
		AM_Compact(1, header->numKeys, pageBuf, tempPage, header);
		memcpy(pageBuf, tempPage, PF_PAGE_SIZE);
		memcpy(header, pageBuf, AM_sl);
		AM_InsertToLeafNotFound(pageBuf, value, recId, index, header);
		header->numKeys++;
		memcpy(pageBuf, header, AM_sl);
		return(TRUE);
	}
	else
		return(FALSE);
}

int AM_InsertToLeafFound(char *pageBuf, int recId, int index, AM_LEAFHEADER *header)
{
	int recSize;
	short tempPtr;
	short oldhead;

	recSize = header->attrLength + AM_ss;
	if ((header->freeListPtr) == 0)
	{
		header->recIdPtr = header->recIdPtr - AM_si - AM_ss;
		tempPtr = header->recIdPtr;
	}
	else 
	{
		tempPtr = header->freeListPtr;
		header->numinfreeList--;
		memcpy((char *)&(header->freeListPtr), pageBuf + tempPtr + AM_si, AM_ss);
	}
	
	memcpy((char *)&oldhead, pageBuf + AM_sl + (index-1)*recSize + header->attrLength, AM_ss);
	memcpy(pageBuf + AM_sl + (index-1)*recSize + header->attrLength, (char *)&tempPtr, AM_ss);
	memcpy(pageBuf + tempPtr, (char *)&recId, AM_si);
	memcpy(pageBuf + tempPtr + AM_si, (char *)&oldhead, AM_ss);
	return 0;
}

int AM_InsertToLeafNotFound(char *pageBuf, char *value, int recId, int index, AM_LEAFHEADER *header)
{
	int recSize;
	short nullVal = AM_NULL;
	int i;

	recSize = header->attrLength + AM_ss;
	for(i = header->numKeys; i >= index; i--)
	{
		memcpy(pageBuf + AM_sl + i*recSize, pageBuf + AM_sl + (i-1)*recSize, recSize);
	} 
	header->keyPtr = header->keyPtr + recSize;
	
	memcpy(pageBuf + AM_sl + (index-1)*recSize, value, header->attrLength);
	memcpy(pageBuf + AM_sl + (index-1)*recSize + header->attrLength, (char *)&nullVal, AM_ss);
	
	AM_InsertToLeafFound(pageBuf, recId, index, header);
	return 0;
}

int AM_Compact(int low, int high, char *pageBuf, char *tempPage, AM_LEAFHEADER *header)
{
	short nextRec;
	AM_LEAFHEADER temphead, *tempheader;
	short recIdPtr;
	int recSize;
	int i, j;
	int offset1, offset2;

	tempheader = &temphead;
	memcpy(tempheader, header, AM_sl);
	
	recSize = header->attrLength + AM_ss;
	recIdPtr = PF_PAGE_SIZE - AM_si - AM_ss ;

	for (i = low, j = 1; i <= high; i++, j++)
	{
		offset1 = (i - 1) * recSize + AM_sl;
		offset2 = (j - 1) * recSize + AM_sl;
		memcpy(tempPage + offset2, pageBuf + offset1, header->attrLength);
		memcpy((char *)&nextRec, pageBuf + offset1 + header->attrLength, AM_ss);
		memcpy(tempPage + offset2 + header->attrLength, (char *)&recIdPtr, AM_ss);
		while (nextRec != 0)
		{
			memcpy(tempPage + recIdPtr, pageBuf + nextRec, AM_si);
			recIdPtr = recIdPtr - AM_si - AM_ss;
			memcpy(tempPage + recIdPtr + 2 * AM_si + AM_ss, (char *)&recIdPtr, AM_ss);
			memcpy((char *)&nextRec, pageBuf + nextRec + AM_si, AM_ss);
		}
		memcpy(tempPage + recIdPtr + 2 * AM_si + AM_ss, (char *)&nextRec, AM_ss);
	}

	tempheader->pageType = header->pageType;
	tempheader->nextLeafPage = header->nextLeafPage;
	tempheader->recIdPtr = recIdPtr + AM_si + AM_ss;
	tempheader->keyPtr = offset2 + recSize;
	tempheader->freeListPtr = 0;
	tempheader->numinfreeList = 0;
	tempheader->attrLength = header->attrLength;
	tempheader->numKeys = high - low + 1;
	tempheader->maxKeys = header->maxKeys;
	memcpy(tempPage, tempheader, AM_sl);
	return 0;
}
