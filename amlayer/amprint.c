#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "am.h"
#include "pf.h"

int AM_PrintAttr(char *bufPtr, char attrType, int attrLength);
int AM_PrintLeafKeys(char *pageBuf, char attrType);

int AM_PrintIntNode(char *pageBuf, char attrType)
{
int tempPageint;
int i;
int recSize;
AM_INTHEADER *header;

header = (AM_INTHEADER *) calloc(1, AM_sint);
memcpy(header, pageBuf, AM_sint);
recSize = header->attrLength + AM_si;
printf("PAGETYPE %c\n", header->pageType);
printf("NUMKEYS %d\n", header->numKeys);
printf("MAXKEYS %d\n", header->maxKeys);
printf("ATTRLENGTH %d\n", header->attrLength);
memcpy((char *)&tempPageint, pageBuf + AM_sint, AM_si);
printf("FIRSTPAGE is %d\n", tempPageint);
for(i = 1 ; i <= (header->numKeys); i++)
  {
   AM_PrintAttr(pageBuf + (i-1)*recSize + AM_sint + AM_si, attrType, header->attrLength);
   memcpy((char *)&tempPageint, pageBuf + i*recSize + AM_sint, AM_si);
   printf("NEXTPAGE is %d\n", tempPageint);
  }
free(header);
return 0;
}

int AM_PrintLeafNode(char *pageBuf, char attrType)
{
short nextRec;
int i;
int recSize;
int recId;
int offset1;
AM_LEAFHEADER *header;

header = (AM_LEAFHEADER *) calloc(1, AM_sl);
memcpy(header, pageBuf, AM_sl);
recSize = header->attrLength + AM_ss;
printf("PAGETYPE %c\n", header->pageType);
printf("NEXTLEAFPAGE %d\n", header->nextLeafPage);
printf("NUMKEYS %d\n", header->numKeys);
for (i = 1; i <= header->numKeys; i++)
  {
  offset1 = (i - 1) * recSize + AM_sl;
  AM_PrintAttr(pageBuf + AM_sl + (i-1)*recSize, attrType, header->attrLength);
  memcpy((char *)&nextRec, pageBuf + offset1 + header->attrLength, AM_ss);
  while (nextRec != 0)
    {
    memcpy((char *)&recId, pageBuf + nextRec, AM_si);
    printf("RECID is %d\n", recId);
    memcpy((char *)&nextRec, pageBuf + nextRec + AM_si, AM_ss);
    }
  printf("\n\n");
  }
free(header);
return 0;
}

int AM_DumpLeafPages(int fileDesc, int min, char attrType, int attrLength)
{
int pageNum;
char *value;
char *pageBuf;
int errVal;
AM_LEAFHEADER *header;

value = malloc(AM_si);
memcpy(value, &min, AM_si);
printf("%d PAGE \n", AM_LeftPageNum);
PF_GetThisPage(fileDesc, AM_LeftPageNum, &pageBuf);
header = (AM_LEAFHEADER *) calloc(1, AM_sl);
memcpy(header, pageBuf, AM_sl);
pageNum = AM_LeftPageNum;
while(header->nextLeafPage != -1)
  {
   printf("PAGENUMBER = %d\n", pageNum);   
   AM_PrintLeafKeys(pageBuf, attrType);
   errVal = PF_UnfixPage(fileDesc, pageNum, FALSE);
   AM_Check;
   pageNum = header->nextLeafPage;   
   errVal = PF_GetThisPage(fileDesc, pageNum, &pageBuf);
   AM_Check;
   memcpy(header, pageBuf, AM_sl);
  }
printf("PAGENUMBER = %d\n", pageNum);
AM_PrintLeafKeys(pageBuf, attrType);
errVal = PF_UnfixPage(fileDesc, pageNum, FALSE);
AM_Check;
free(value);
free(header);
return 0;
}

int AM_PrintLeafKeys(char *pageBuf, char attrType)
{
short nextRec;
int i;
int recSize;
int recId;
int offset1;
AM_LEAFHEADER *header;

header = (AM_LEAFHEADER *) calloc(1, AM_sl);
memcpy(header, pageBuf, AM_sl);
recSize = header->attrLength + AM_ss;
for (i = 1; i <= header->numKeys; i++)
  {
  offset1 = (i - 1) * recSize + AM_sl;
  AM_PrintAttr(pageBuf + AM_sl + (i-1)*recSize, attrType, header->attrLength);
  memcpy((char *)&nextRec, pageBuf + offset1 + header->attrLength, AM_ss);
  while (nextRec != 0)
    {
    memcpy((char *)&recId, pageBuf + nextRec, AM_si);
    printf("RECID is %d\n", recId);
    memcpy((char *)&nextRec, pageBuf + nextRec + AM_si, AM_ss);
    }
  }
free(header);
return 0;
}

int AM_PrintAttr(char *bufPtr, char attrType, int attrLength)
{
int bufint;
float buffloat;
char *bufstr;

switch(attrType)
  {
   case 'i': {
               memcpy((char *)&bufint, bufPtr, AM_si);
               printf("ATTRIBUTE is %d\n", bufint);
               break;
              }
   case 'f': {
               memcpy((char *)&buffloat, bufPtr, AM_sf);
               printf("ATTRIBUTE is %f\n", buffloat);
               break;
              }
   case 'c': {
	       bufstr = malloc((unsigned) (attrLength + 1));
               memcpy(bufstr, bufPtr, attrLength);
               bufstr[attrLength] = '\0';
               printf("ATTRIBUTE is %s\n", bufstr);
               free(bufstr);
	       break;
              }
   }
return 0;
}

int AM_PrintTree(int fileDesc, int pageNum, char attrType)
{
int nextPage;
int errVal;
AM_INTHEADER *header;
char *tempPage;
char *pageBuf;
int recSize;
int i;

printf("GETTING PAGE = %d\n", pageNum);
errVal = PF_GetThisPage(fileDesc, pageNum, &pageBuf);
tempPage = malloc(PF_PAGE_SIZE);
memcpy(tempPage, pageBuf, PF_PAGE_SIZE);
errVal = PF_UnfixPage(fileDesc, pageNum, FALSE);
if (*tempPage == 'l')
  {
   printf("PAGENUM = %d\n", pageNum);
   AM_PrintLeafKeys(tempPage, attrType);
   free(tempPage);
   return 0;
  }
header = (AM_INTHEADER *)calloc(1, AM_sint);
memcpy(header, tempPage, AM_sint);
recSize = header->attrLength + AM_si;
for(i = 1; i <= (header->numKeys + 1); i++)
  {
   memcpy((char *)&nextPage, tempPage + AM_sint + (i-1)*recSize, AM_si);
   AM_PrintTree(fileDesc, nextPage, attrType);
  }
printf("PAGENUM = %d\n", pageNum);
AM_PrintIntNode(tempPage, attrType);
free(header);
free(tempPage);
return 0;
}
