#include <stdio.h>
#include <string.h>
#include "pf.h"
#include "am.h"

int AM_CreateIndex(char *fileName, int indexNo, char attrType, int attrLength)
{
	char *pageBuf;
	char indexfName[AM_MAX_FNAME_LENGTH];
	int pageNum;
	int fileDesc;
	int errVal;
	int maxKeys;
	AM_LEAFHEADER head, *header;

	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
		AM_Errno = AME_INVALIDATTRTYPE;
		return(AME_INVALIDATTRTYPE);
	}
        
	if ((attrLength < 1) || (attrLength > 255))
	{
		AM_Errno = AME_INVALIDATTRLENGTH;
		return(AME_INVALIDATTRLENGTH);
	}
	
	if (attrLength != 4)
		if (attrType != 'c')
		{
			AM_Errno = AME_INVALIDATTRLENGTH;
			return(AME_INVALIDATTRLENGTH);
		}
	
	header = &head;
	
	snprintf(indexfName, sizeof(indexfName), "%s.%d", fileName, indexNo);
	errVal = PF_CreateFile(indexfName);
	AM_Check;

	fileDesc = PF_OpenFile(indexfName);
	if (fileDesc < 0) 
	{
		AM_Errno = AME_PF;
		return(AME_PF);
	}

	errVal = PF_AllocPage(fileDesc, &pageNum, &pageBuf);
	AM_Check;
	
	header->pageType = 'l';
	header->nextLeafPage = AM_NULL_PAGE;
	header->recIdPtr = PF_PAGE_SIZE;
	header->keyPtr = AM_sl;
	header->freeListPtr = AM_NULL;
	header->numinfreeList = 0;
	header->attrLength = attrLength;
	header->numKeys = 0;
	
	maxKeys = (PF_PAGE_SIZE - AM_sint - AM_si)/(AM_si + attrLength);
	if ((maxKeys % 2) != 0) 
		header->maxKeys = maxKeys - 1;
	else 
		header->maxKeys = maxKeys;

	memcpy(pageBuf, header, AM_sl);
	
	errVal = PF_UnfixPage(fileDesc, pageNum, TRUE);
	AM_Check;
	
	errVal = PF_CloseFile(fileDesc);
	AM_Check;
	
	AM_RootPageNum = pageNum;
	return(AME_OK);
}

int AM_DestroyIndex(char *fileName, int indexNo)
{
	char indexfName[AM_MAX_FNAME_LENGTH];
	int errVal;

	snprintf(indexfName, sizeof(indexfName), "%s.%d", fileName, indexNo);
	errVal = PF_DestroyFile(indexfName);
	AM_Check;
	return(AME_OK);
}

int AM_DeleteEntry(int fileDesc, char attrType, int attrLength, char *value, int recId)
{
	char *pageBuf;
	int pageNum;
	int index;
	int status;
	short nextRec;
	short oldhead;
	short temp; 
	char *currRecPtr;
	AM_LEAFHEADER head, *header;
	int recSize;
	int tempRec;
	int errVal;
	int i;

	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
		AM_Errno = AME_INVALIDATTRTYPE;
		return(AME_INVALIDATTRTYPE);
	}

	if (value == NULL) 
	{
		AM_Errno = AME_INVALIDVALUE;
		return(AME_INVALIDVALUE);
	}

	if (fileDesc < 0) 
	{
		AM_Errno = AME_FD;
		return(AME_FD);
	}

	header = &head;
	
	status = AM_Search(fileDesc, attrType, attrLength, value, &pageNum, &pageBuf, &index);
	
	if (status < 0) 
	{
		AM_Errno = status;
		return(status);
	}
	
	if (status == AM_NOT_FOUND) 
	{
		AM_Errno = AME_NOTFOUND;
		return(AME_NOTFOUND);
	}
	
	memcpy(header, pageBuf, AM_sl);
	recSize = attrLength + AM_ss;
	currRecPtr = pageBuf + AM_sl + (index - 1)*recSize + attrLength;
	memcpy((char *)&nextRec, currRecPtr, AM_ss);
	
	while(nextRec != 0)
	{
		memcpy((char *)&tempRec, pageBuf + nextRec, AM_si);
		
		if (recId == tempRec)
		{
			memcpy(currRecPtr, pageBuf + nextRec + AM_si, AM_ss);
			header->numinfreeList++;
			oldhead = header->freeListPtr;
			header->freeListPtr = nextRec;
			memcpy(pageBuf + nextRec + AM_si, (char *)&oldhead, AM_ss);
			break;
		}
		else 
		{
			currRecPtr = pageBuf + nextRec + AM_si;
			memcpy((char *)&nextRec, currRecPtr, AM_ss);
		}
	}
	
	if (nextRec == AM_NULL)
	{
		AM_Errno = AME_NOTFOUND;
		return(AME_NOTFOUND);
	}
	
	memcpy((char *)&temp, pageBuf + AM_sl + (index - 1)*recSize + attrLength, AM_ss);
	if (temp == 0)
	{
		for(i = index; i < (header->numKeys); i++)
			memcpy(pageBuf + AM_sl + (i-1)*recSize, pageBuf + AM_sl + i*recSize, recSize);
		header->numKeys--;
		header->keyPtr = header->keyPtr - recSize;
	}
	
	memcpy(pageBuf, header, AM_sl);
	
	errVal = PF_UnfixPage(fileDesc, pageNum, TRUE);
	
	AM_EmptyStack();
	AM_Errno = AME_OK;
	return(AME_OK);
}

int AM_InsertEntry(int fileDesc, char attrType, int attrLength, char *value, int recId)
{
	char *pageBuf;
	int pageNum;
	int index;
	int status;
	int inserted;
	int addtoparent;
	int errVal;
	char key[AM_MAXATTRLENGTH];

	if ((attrType != 'c') && (attrType != 'f') && (attrType != 'i'))
	{
		AM_Errno = AME_INVALIDATTRTYPE;
		return(AME_INVALIDATTRTYPE);
	}

	if (value == NULL) 
	{
		AM_Errno = AME_INVALIDVALUE;
		return(AME_INVALIDVALUE);
	}

	if (fileDesc < 0) 
	{
		AM_Errno = AME_FD;
		return(AME_FD);
	}
	
	status = AM_Search(fileDesc, attrType, attrLength, value, &pageNum, &pageBuf, &index);

	if (status < 0) 
	{ 
		AM_EmptyStack();
		AM_Errno = status;
		return(status);
	}
	
	inserted = AM_InsertintoLeaf(pageBuf, attrLength, value, recId, index, status);

	if (inserted == TRUE) 
	{
		errVal = PF_UnfixPage(fileDesc, pageNum, TRUE);
		AM_Check;
		AM_EmptyStack();
		return(AME_OK);
	}
	
	if (inserted < 0) 
	{
		AM_EmptyStack();
		AM_Errno = inserted;
		return(inserted);
	}
	
	if (inserted == FALSE)
	{
		addtoparent = AM_SplitLeaf(fileDesc, pageBuf, &pageNum, attrLength, recId, value, status, index, key);
		
		if (addtoparent < 0) 
		{
			AM_EmptyStack();
			AM_Errno = addtoparent;
			return(addtoparent);
		}
		
		if (addtoparent == TRUE)
		{
			errVal = AM_AddtoParent(fileDesc, pageNum, key, attrLength);
			if (errVal < 0)
			{
				AM_EmptyStack();
				AM_Errno = errVal;
				return(errVal);
			}
		}
	}
	AM_EmptyStack();
	return(AME_OK);
}

static char *AMerrormsg[] = {
"No error",
"Invalid Attribute Length",
"Key Not Found in Tree",
"PF error",
"Internal error - contact database manager immediately",
"Invalid scan Descriptor",
"Invalid operator to OpenIndexScan",
"Scan Over",
"Scan Table is full",
"Invalid Attribute Type",
"Invalid file Descriptor",
"Invalid value to Delete or Insert Entry"
};

void AM_PrintError(char *s)
{
   fprintf(stderr, "%s", s);
   fprintf(stderr, "%s", AMerrormsg[-1*AM_Errno]);
   if (AM_Errno == AME_PF)
      PF_PrintError(" ");
   else 
     fprintf(stderr, "\n");
}
