#include <stdio.h>
#include <string.h>
#include "am.h"
#include "pf.h"

/* splits a leaf node */
int AM_SplitLeaf(int fileDesc, char *pageBuf, int *pageNum, int attrLength, int recId, char *value, int status, int index, char *key)
{
	AM_LEAFHEADER head, temphead;
	AM_LEAFHEADER *header, *tempheader;
	char tempPage[PF_PAGE_SIZE];
	char *tempPageBuf, *tempPageBuf1;
	int errVal; 
	int tempPageNum, tempPageNum1;

	header = &head;
	tempheader = &temphead;

	memcpy(header, pageBuf, AM_sl);

	AM_Compact(1, (header->numKeys)/2, pageBuf, tempPage, header);

	errVal = PF_AllocPage(fileDesc, &tempPageNum, &tempPageBuf);
	AM_Check;

	AM_Compact((header->numKeys)/2 + 1, header->numKeys, pageBuf, tempPageBuf, header);

	if (index <= ((header->numKeys)/2))
	{
		errVal = AM_InsertintoLeaf(tempPage, attrLength, value, recId, index, status);
	}
	else
	{
		index = index - ((header->numKeys)/2);
		errVal = AM_InsertintoLeaf(tempPageBuf, attrLength, value, recId, index, status);
	}

	memcpy(tempheader, tempPage, AM_sl);
	tempheader->nextLeafPage = tempPageNum;
	memcpy(tempPage, tempheader, AM_sl);
	memcpy(pageBuf, tempPage, PF_PAGE_SIZE);

	memcpy(key, tempPageBuf + AM_sl, attrLength);

	if ((*pageNum) == AM_RootPageNum)
	{
		errVal = PF_AllocPage(fileDesc, &tempPageNum1, &tempPageBuf1);
		AM_Check;

		AM_LeftPageNum = tempPageNum1;

		memcpy(tempPageBuf1, pageBuf, PF_PAGE_SIZE);

		AM_FillRootPage(pageBuf, tempPageNum1, tempPageNum, key, header->attrLength, header->maxKeys);
		errVal = PF_UnfixPage(fileDesc, tempPageNum1, TRUE);
		AM_Check;
	}

	errVal = PF_UnfixPage(fileDesc, *pageNum, TRUE);
	AM_Check;

	errVal = PF_UnfixPage(fileDesc, tempPageNum, TRUE);
	AM_Check;

	if ((*pageNum) == AM_RootPageNum)
		return(FALSE);
	else
	{
		*pageNum = tempPageNum;
		return(TRUE);
	}
}

int AM_AddtoParent(int fileDesc, int pageNum, char *value, int attrLength)
{
	char tempPage[PF_PAGE_SIZE];
	int pageNumber;
	int offset;
	int errVal; 
	int pageNum1, pageNum2;
	char *pageBuf, *pageBuf1, *pageBuf2;
	AM_INTHEADER head, *header;

	header = &head;
	AM_topofStack(&pageNumber, &offset);
	AM_PopStack();

	errVal = PF_GetThisPage(fileDesc, pageNumber, &pageBuf);
	AM_Check;

	memcpy(header, pageBuf, AM_sint);

	if ((header->numKeys) < (header->maxKeys))
	{
		AM_AddtoIntPage(pageBuf, value, pageNum, header, offset);
		memcpy(pageBuf, header, AM_sint);

		errVal = PF_UnfixPage(fileDesc, pageNumber, TRUE);
		AM_Check;
		return(AME_OK);
	}
	else
	{
		errVal = PF_AllocPage(fileDesc, &pageNum1, &pageBuf1);
		AM_Check;

		AM_SplitIntNode(pageBuf, tempPage, pageBuf1, header, value, pageNum, offset);

		if (pageNumber == AM_RootPageNum)
		{
			errVal = PF_AllocPage(fileDesc, &pageNum2, &pageBuf2);
			AM_Check;

			memcpy(pageBuf2, tempPage, PF_PAGE_SIZE);

			AM_FillRootPage(pageBuf, pageNum2, pageNum1, value, header->attrLength, header->maxKeys);

			errVal = PF_UnfixPage(fileDesc, pageNumber, TRUE);
			AM_Check;

			errVal = PF_UnfixPage(fileDesc, pageNum1, TRUE);
			AM_Check;

			errVal = PF_UnfixPage(fileDesc, pageNum2, TRUE);
			AM_Check;

			return(AME_OK);
		}
		else
		{
			memcpy(pageBuf, tempPage, PF_PAGE_SIZE);

			errVal = PF_UnfixPage(fileDesc, pageNumber, TRUE);
			AM_Check;

			errVal = PF_UnfixPage(fileDesc, pageNum1, TRUE);
			AM_Check;

			errVal = AM_AddtoParent(fileDesc, pageNum1, value, attrLength);
			AM_Check;
		}
	}
	return(AME_OK);
}

int AM_AddtoIntPage(char *pageBuf, char *value, int pageNum, AM_INTHEADER *header, int offset)
{
	int recSize;
	int i;

	recSize = header->attrLength + AM_si;

	for(i = header->numKeys; i > offset; i--)
		memcpy(pageBuf + AM_sint + i*recSize + AM_si, pageBuf + AM_sint + (i-1)*recSize + AM_si, recSize);

	memcpy(pageBuf + AM_sint + offset*recSize + AM_si, value, header->attrLength);
	memcpy(pageBuf + AM_sint + (offset+1)*recSize, (char *)&pageNum, AM_si);

	header->numKeys++;
	return 0;
}

int AM_FillRootPage(char *pageBuf, int pageNum1, int pageNum2, char *value, short attrLength, short maxKeys)
{
	AM_INTHEADER temphead, *tempheader;

	tempheader = &temphead;

	tempheader->pageType = 'i';
	tempheader->attrLength = attrLength;
	tempheader->maxKeys = maxKeys;
	tempheader->numKeys = 1;
	memcpy(pageBuf + AM_sint, (char *)&pageNum1, AM_si);
	memcpy(pageBuf + AM_sint + AM_si, value, attrLength);
	memcpy(pageBuf + AM_sint + AM_si + attrLength, (char *)&pageNum2, AM_si);
	memcpy(pageBuf, tempheader, AM_sint);
	return 0;
}

int AM_SplitIntNode(char *pageBuf, char *pbuf1, char *pbuf2, AM_INTHEADER *header, char *value, int pageNum, int offset)
{
	AM_INTHEADER temphead, *tempheader;
	int recSize;
	char tempPage[PF_PAGE_SIZE + AM_MAXATTRLENGTH];
	int length1, length2;

	tempheader = &temphead;
	recSize = header->attrLength + AM_si;

	tempheader->pageType = header->pageType;
	tempheader->attrLength = header->attrLength;
	tempheader->maxKeys = header->maxKeys;

	length1 = AM_si + (offset*recSize);
	memcpy(tempPage, pageBuf + AM_sint, length1);
	memcpy(tempPage + length1, value, header->attrLength);
	memcpy(tempPage + length1 + header->attrLength, (char *)&pageNum, AM_si);

	length2 = (header->maxKeys - offset)*recSize;
	memcpy(tempPage + length1 + header->attrLength + AM_si, pageBuf + AM_sint + length1, length2);

	length1 = (header->maxKeys)/2;
	length2 = AM_si + length1*recSize;

	memcpy(pbuf1 + AM_sint, tempPage, length2);
	tempheader->numKeys = length1;
	memcpy(pbuf1, tempheader, AM_sint);

	memcpy(value, tempPage + AM_si + length1 * recSize, header->attrLength);

	memcpy(pbuf2 + AM_sint, tempPage + AM_si + length1 * recSize + header->attrLength, length2); 
	memcpy(pbuf2, tempheader, AM_sint);
	return 0;
}
