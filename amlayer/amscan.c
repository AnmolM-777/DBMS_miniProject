#include <stdio.h>
#include <string.h>
#include "am.h"
#include "pf.h"

static struct {
         int fileDesc;
         int op;
         int attrType;
         int pageNum;
         short index;
         short actindex;
         int nextpageNum;
         char nextvalue[AM_MAXATTRLENGTH];
         short nextIndex;
         short nextRecIdPtr;
         int lastpageNum;
         short lastIndex;
         int status;
} AM_scanTable[MAXSCANS];

int AM_OpenIndexScan(int fileDesc, char attrType, int attrLength, int op, char *value)
{
int scanDesc;
int status;
int index;
int pageNum;
int recSize;
char *pageBuf;
int errVal;
AM_LEAFHEADER head, *header;
int searchpageNum;

if (fileDesc < 0)
  {
   AM_Errno = AME_FD;
   return(AME_FD);
  }

if ((attrType != 'i') && (attrType != 'c') && (attrType != 'f'))
  {
  AM_Errno = AME_INVALIDATTRTYPE;
  return(AME_INVALIDATTRTYPE);
  }

header = &head;

for (scanDesc = 0; scanDesc < MAXSCANS; scanDesc++)
  if (AM_scanTable[scanDesc].status == FREE) break;

if (scanDesc > MAXSCANS - 1) 
 {
 AM_Errno = AME_SCAN_TAB_FULL;
 return(AME_SCAN_TAB_FULL);
 }

AM_scanTable[scanDesc].status = FIRST;
AM_scanTable[scanDesc].attrType = attrType;

AM_LeftPageNum = GetLeftPageNum(fileDesc);

if (value == NULL)
  {
   AM_scanTable[scanDesc].fileDesc = fileDesc;
   AM_scanTable[scanDesc].op = ALL;
   AM_scanTable[scanDesc].nextpageNum = AM_LeftPageNum;
   AM_scanTable[scanDesc].nextIndex = 1;
   AM_scanTable[scanDesc].actindex = 1;
   errVal = PF_GetThisPage(fileDesc, AM_LeftPageNum, &pageBuf);
   AM_Check;
   memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + attrLength, AM_ss);
   errVal = PF_UnfixPage(fileDesc, AM_LeftPageNum, FALSE);
   AM_Check;
   return(scanDesc);
  }
  
status = AM_Search(fileDesc, attrType, attrLength, value, &pageNum, &pageBuf, &index);
searchpageNum = pageNum;
if (status < 0) 
  { AM_scanTable[scanDesc].status = FREE;
    AM_Errno = status;
    return(status);
  }

memcpy(header, pageBuf, AM_sl);
recSize = attrLength + AM_ss;
AM_scanTable[scanDesc].fileDesc = fileDesc;
AM_scanTable[scanDesc].op = op;

if (index > header->numKeys) 
  if (header->nextLeafPage != AM_NULL_PAGE)
  {
  errVal = PF_GetThisPage(fileDesc, header->nextLeafPage, &pageBuf);
  AM_Check;
  memcpy(header, pageBuf, AM_sl);
  errVal = PF_UnfixPage(fileDesc, header->nextLeafPage, FALSE);
  AM_Check;
  pageNum = header->nextLeafPage;
  index = 1;
  }
  else 
   pageNum = AM_NULL_PAGE;

AM_scanTable[scanDesc].pageNum = pageNum;
AM_scanTable[scanDesc].index = index;

switch(op) 
 {
  case EQUAL : 
               {
		if (status != AM_FOUND)
                   AM_scanTable[scanDesc].status = OVER;
                else
                 {
                  AM_scanTable[scanDesc].nextpageNum = pageNum;
                  AM_scanTable[scanDesc].nextIndex = index;
                  AM_scanTable[scanDesc].actindex = index;
                  memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (index - 1)*recSize + attrLength, AM_ss);
                  AM_scanTable[scanDesc].lastpageNum = pageNum;
                  AM_scanTable[scanDesc].lastIndex = index;
                 }
                break;
               }
  case LESS_THAN : 
               {
                AM_scanTable[scanDesc].nextpageNum = AM_LeftPageNum;
                AM_scanTable[scanDesc].nextIndex = 1;
                AM_scanTable[scanDesc].actindex = 1;
                if (searchpageNum != AM_LeftPageNum)
		 { errVal = PF_GetThisPage(fileDesc, AM_LeftPageNum, &pageBuf);
                  AM_Check;
                 }
                memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + attrLength, AM_ss);
                if (searchpageNum != AM_LeftPageNum)
		 {
		  errVal = PF_UnfixPage(fileDesc, AM_LeftPageNum, FALSE);
                  AM_Check;
                 }
                AM_scanTable[scanDesc].lastpageNum = pageNum;
                AM_scanTable[scanDesc].lastIndex = index - 1;
                break;
               }
  case GREATER_THAN :
               {
                if (status == AM_FOUND)
                 if ((index + 1) <= (header->numKeys))
                  {
                   AM_scanTable[scanDesc].nextpageNum = pageNum;
                   AM_scanTable[scanDesc].nextIndex = index + 1;
                   AM_scanTable[scanDesc].actindex = index + 1;
                   memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (index)*recSize + attrLength, AM_ss);
                  }
                 else
		   if (header->nextLeafPage != AM_NULL_PAGE)
                   {
                   AM_scanTable[scanDesc].nextpageNum = header->nextLeafPage;
                   AM_scanTable[scanDesc].nextIndex = 1;
                   AM_scanTable[scanDesc].actindex = 1;
                   errVal = PF_GetThisPage(fileDesc, header->nextLeafPage, &pageBuf);
                   AM_Check;
                   memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + attrLength, AM_ss);
                   errVal = PF_UnfixPage(fileDesc, header->nextLeafPage, FALSE);
                   AM_Check;
                   }
                   else
                     AM_scanTable[scanDesc].status = OVER;
                else 
                 {
                   AM_scanTable[scanDesc].nextpageNum = pageNum;
                   AM_scanTable[scanDesc].nextIndex = index;
                   AM_scanTable[scanDesc].actindex = index;
                   memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (index - 1)*recSize + attrLength, AM_ss);
                 }
                break;
               }
  case LESS_THAN_EQUAL :
               {
               AM_scanTable[scanDesc].nextpageNum = AM_LeftPageNum;
               AM_scanTable[scanDesc].nextIndex = 1;
               AM_scanTable[scanDesc].actindex = 1;
                if (searchpageNum != AM_LeftPageNum)
		 { errVal = PF_GetThisPage(fileDesc, AM_LeftPageNum, &pageBuf);
                  AM_Check;
                 }
               memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + attrLength, AM_ss);
               if (searchpageNum != AM_LeftPageNum)
		 {
		  errVal = PF_UnfixPage(fileDesc, AM_LeftPageNum, FALSE);
                  AM_Check;
                 }		   
               AM_scanTable[scanDesc].lastpageNum = pageNum;
               if (status == AM_FOUND)
                AM_scanTable[scanDesc].lastIndex = index;
               else
                AM_scanTable[scanDesc].lastIndex = index - 1;
               break;
               }
  case GREATER_THAN_EQUAL :
               {
                AM_scanTable[scanDesc].nextpageNum = pageNum;
                AM_scanTable[scanDesc].nextIndex = index;
                AM_scanTable[scanDesc].actindex = index;
                memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (index - 1)*recSize + attrLength, AM_ss);
                break;
               }
  case NOT_EQUAL :
               {
               if(status == AM_FOUND)
                {
                AM_scanTable[scanDesc].nextpageNum = AM_LeftPageNum;
                AM_scanTable[scanDesc].nextIndex = 1;
                AM_scanTable[scanDesc].actindex = 1;
                if (searchpageNum != AM_LeftPageNum)
		  {
		  errVal = PF_GetThisPage(fileDesc, AM_LeftPageNum, &pageBuf);
                  AM_Check;
		  }
                memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + attrLength, AM_ss);
                if (searchpageNum != AM_LeftPageNum)
		 { errVal = PF_UnfixPage(fileDesc, AM_LeftPageNum, FALSE);
                  AM_Check;
                 }
                }
               else 
                AM_scanTable[scanDesc].pageNum = AM_NULL_PAGE;
               break;
               }
  default : {
             AM_scanTable[scanDesc].status = FREE;
	     AM_Errno = AME_INVALID_OP_TO_SCAN;
	     return(AME_INVALID_OP_TO_SCAN);
             break;
            }
 }

errVal = PF_UnfixPage(fileDesc, searchpageNum, FALSE);
AM_Check;
return(scanDesc);
}

int AM_FindNextEntry(int scanDesc)
{
int recId;
char *pageBuf;
int errVal;
AM_LEAFHEADER head, *header;
int recSize;
int compareVal;

if ((scanDesc < 0) || (scanDesc > MAXSCANS - 1))
  {
   AM_Errno = AME_INVALID_SCANDESC;
   return(AME_INVALID_SCANDESC);
  }

if (AM_scanTable[scanDesc].status == OVER)
      return(AME_EOF);

/* Equality scans can read the RID overflow chain directly. This avoids
   carrying a page-buffer pointer across unfix operations in the generic scan. */
if (AM_scanTable[scanDesc].op == EQUAL) {
    short ptr, nextPtr;
    if (AM_scanTable[scanDesc].nextpageNum == AM_NULL_PAGE || AM_scanTable[scanDesc].nextRecIdPtr == 0) {
        AM_scanTable[scanDesc].status = OVER;
        return AME_EOF;
    }
    errVal = PF_GetThisPage(AM_scanTable[scanDesc].fileDesc, AM_scanTable[scanDesc].nextpageNum, &pageBuf);
    AM_Check;
    ptr = AM_scanTable[scanDesc].nextRecIdPtr;
    memcpy(&recId, pageBuf + ptr, AM_si);
    memcpy(&nextPtr, pageBuf + ptr + AM_si, AM_ss);
    AM_scanTable[scanDesc].nextRecIdPtr = nextPtr;
    if (nextPtr == 0) AM_scanTable[scanDesc].status = OVER;
    errVal = PF_UnfixPage(AM_scanTable[scanDesc].fileDesc, AM_scanTable[scanDesc].nextpageNum, FALSE);
    AM_Check;
    return recId;
}

if (AM_scanTable[scanDesc].nextpageNum == AM_NULL_PAGE)
 {
  AM_scanTable[scanDesc].status = OVER;
  return(AME_EOF);
 }

header = &head;
errVal = PF_GetThisPage(AM_scanTable[scanDesc].fileDesc, AM_scanTable[scanDesc].nextpageNum, &pageBuf);
AM_Check;

memcpy(header, pageBuf, AM_sl);
recSize = header->attrLength + AM_ss;

errVal = PF_UnfixPage(AM_scanTable[scanDesc].fileDesc, AM_scanTable[scanDesc].nextpageNum, FALSE);
AM_Check;

while(header->numKeys == 0)
  if(header->nextLeafPage == AM_NULL_PAGE)
   {
    AM_scanTable[scanDesc].status = OVER; 
    return(AME_EOF);
   }
  else
   {
    errVal = PF_GetThisPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, &pageBuf);
    AM_Check;
    errVal = PF_UnfixPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, FALSE);
    AM_Check;
    AM_scanTable[scanDesc].nextpageNum = header->nextLeafPage;
    AM_scanTable[scanDesc].nextIndex = 1;
    AM_scanTable[scanDesc].actindex = 1;
    memcpy(header, pageBuf, AM_sl);
    memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex-1)*recSize + header->attrLength, AM_ss);
    AM_scanTable[scanDesc].status = FIRST;
   }

if ((AM_scanTable[scanDesc].op == LESS_THAN) || 
  (AM_scanTable[scanDesc].op == LESS_THAN_EQUAL))
 if ((AM_scanTable[scanDesc].lastpageNum <= AM_scanTable[scanDesc].nextpageNum)
 && (AM_scanTable[scanDesc].lastIndex == 0))
 {
  AM_scanTable[scanDesc].status = OVER;
  return(AME_EOF);
 }

if (AM_scanTable[scanDesc].op == NOT_EQUAL)
  if ((AM_scanTable[scanDesc].pageNum == AM_scanTable[scanDesc].nextpageNum)
    && (AM_scanTable[scanDesc].index == AM_scanTable[scanDesc].actindex))
       
       if ((AM_scanTable[scanDesc].nextIndex + 1) <= (header->numKeys))
         {
          AM_scanTable[scanDesc].nextIndex++;
          AM_scanTable[scanDesc].actindex++;
          memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex-1)*recSize + header->attrLength, AM_ss);
         }
       else
          if (header->nextLeafPage == AM_NULL_PAGE)
            return(AME_EOF); 
          else
           {
            AM_scanTable[scanDesc].nextpageNum = header->nextLeafPage;
            AM_scanTable[scanDesc].nextIndex = 1;
            AM_scanTable[scanDesc].actindex = 1;
            errVal = PF_GetThisPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, &pageBuf);
            AM_Check;
            memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + header->attrLength, AM_ss);
            memcpy(header, pageBuf, AM_sl);
            errVal = PF_UnfixPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, FALSE);
            AM_Check;
           }

if (AM_scanTable[scanDesc].status != FIRST)
 {
  compareVal = AM_Compare(pageBuf + (AM_scanTable[scanDesc].nextIndex - 1)*recSize + AM_sl, AM_scanTable[scanDesc].attrType, header->attrLength, AM_scanTable[scanDesc].nextvalue);
  if (compareVal != 0)
   {
    AM_scanTable[scanDesc].nextIndex--;
    memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex -1)*recSize + header->attrLength, AM_ss);
   }
 }
else 
  { AM_scanTable[scanDesc].status = BUSY;
    memcpy(AM_scanTable[scanDesc].nextvalue, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex - 1)*recSize, header->attrLength);
  }

memcpy((char *)&recId, pageBuf + AM_scanTable[scanDesc].nextRecIdPtr, AM_si);

memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_scanTable[scanDesc].nextRecIdPtr + AM_si, AM_ss);

if (AM_scanTable[scanDesc].nextRecIdPtr == (short)0)
   if ((AM_scanTable[scanDesc].nextIndex + 1) <= (header->numKeys))
    {
     AM_scanTable[scanDesc].nextIndex++;
     AM_scanTable[scanDesc].actindex++;
     memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex - 1)*recSize + header->attrLength, AM_ss);
     memcpy(AM_scanTable[scanDesc].nextvalue, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex -1 )*recSize, header->attrLength); 
    }
   else
    if (header->nextLeafPage == AM_NULL_PAGE)
      AM_scanTable[scanDesc].status = OVER;
    else
     {
      AM_scanTable[scanDesc].nextpageNum = header->nextLeafPage;
      AM_scanTable[scanDesc].nextIndex = 1;
      AM_scanTable[scanDesc].actindex = 1;
      errVal = PF_GetThisPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, &pageBuf);
      AM_Check;
      memcpy((char *)&AM_scanTable[scanDesc].nextRecIdPtr, pageBuf + AM_sl + header->attrLength, AM_ss);
      errVal = PF_UnfixPage(AM_scanTable[scanDesc].fileDesc, header->nextLeafPage, FALSE);
      AM_Check;
      memcpy(AM_scanTable[scanDesc].nextvalue, pageBuf + AM_sl + (AM_scanTable[scanDesc].nextIndex -1 )*recSize, header->attrLength); 
      memcpy(header, pageBuf, AM_sl);
     }

if (AM_scanTable[scanDesc].op == EQUAL)
  if ((AM_scanTable[scanDesc].pageNum != AM_scanTable[scanDesc].nextpageNum)
    || (AM_scanTable[scanDesc].index != AM_scanTable[scanDesc].actindex))
     AM_scanTable[scanDesc].status = OVER;

if ((AM_scanTable[scanDesc].op == LESS_THAN) || 
  (AM_scanTable[scanDesc].op == LESS_THAN_EQUAL))
   if ((AM_scanTable[scanDesc].lastpageNum == AM_scanTable[scanDesc].nextpageNum)
    && (AM_scanTable[scanDesc].lastIndex == AM_scanTable[scanDesc].actindex))
       AM_scanTable[scanDesc].status = LAST;
  else if ((AM_scanTable[scanDesc].lastpageNum == AM_scanTable[scanDesc].nextpageNum)
    && (AM_scanTable[scanDesc].lastIndex < AM_scanTable[scanDesc].actindex))
        AM_scanTable[scanDesc].status = OVER;
    else
      if (AM_scanTable[scanDesc].status == LAST)
        AM_scanTable[scanDesc].status = OVER;
        
return(recId);
}

int AM_CloseIndexScan(int scanDesc)
{
if ((scanDesc < 0) || (scanDesc > MAXSCANS - 1))
  {
   AM_Errno = AME_INVALID_SCANDESC;
   return(AME_INVALID_SCANDESC);
  }
AM_scanTable[scanDesc].status = FREE;
return(AME_OK);
}

int GetLeftPageNum(int fileDesc)
{
char *pageBuf;
int pageNum=-1;
int errVal;

errVal = PF_GetFirstPage(fileDesc, &pageNum, &pageBuf);
AM_Check;
if (*pageBuf == 'l')
  AM_LeftPageNum = pageNum;
else
  AM_LeftPageNum = 2;
errVal = PF_UnfixPage(fileDesc, pageNum, FALSE);
AM_Check;
return(AM_LeftPageNum);
}
