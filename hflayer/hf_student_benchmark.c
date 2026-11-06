/* Space-utilization experiment on the supplied semicolon-delimited student file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pf.h"
#include "hf.h"
#define MAX_LINE 4096
int main(int argc,char **argv) {
    const char *path=argc>1?argv[1]:"../data/student.txt";
    FILE *in,*csv; char line[MAX_LINE]; int fd, n=0, maxPage=-1, len, scan, scanCount=0;
    RID rid; char rec[MAX_LINE]; int recLen; unsigned long payload=0;
    const int maxima[]={112,128,160,192,256}; int k;
    PF_Init(); HF_DestroyFile("student_heap.db");
    if(HF_CreateFile("student_heap.db")!=PFE_OK){fprintf(stderr,"cannot create heap file\n");return 1;}
    fd=HF_OpenFile("student_heap.db"); if(fd<0)return 1;
    in=fopen(path,"r"); if(!in){perror(path);return 1;} fgets(line,sizeof(line),in);
    while(fgets(line,sizeof(line),in)) {
        len=(int)strcspn(line,"\r\n"); if(len==0)continue;
        if(HF_InsertRec(fd,line,len,&rid)!=HFE_OK){fprintf(stderr,"insert failed at row %d\n",n);return 1;}
        if(rid.pageNum>maxPage) maxPage=rid.pageNum;
        payload+=(unsigned long)len;
        n++;
    }
    fclose(in);
    scan=HF_OpenScan(fd); while(HF_FindNextRec(scan,&rid,rec,&recLen)==HFE_OK) scanCount++; HF_CloseScan(scan);
    if(HF_CloseFile(fd)!=PFE_OK)return 1;
    csv=fopen("hf_space_results.csv","w"); if(!csv)return 1;
    fprintf(csv,"management,max_record_length,records,pages,payload_bytes,allocated_bytes,utilization_percent,scan_records\n");
    {
      unsigned long pages=(unsigned long)maxPage+1, alloc=pages*PF_PAGE_SIZE;
      unsigned long slottedUsed=payload+(unsigned long)n*sizeof(short)*2+pages*6;
      fprintf(csv,"slotted,variable,%d,%lu,%lu,%lu,%.4f,%d\n",n,pages,payload,alloc,alloc?100.0*slottedUsed/alloc:0.0,scanCount);
    }
    for(k=0;k<5;k++) {
      unsigned long slotsPerPage=PF_PAGE_SIZE/(unsigned long)maxima[k];
      unsigned long pages=(n+slotsPerPage-1)/slotsPerPage, alloc=pages*PF_PAGE_SIZE;
      fprintf(csv,"static,%d,%d,%lu,%lu,%lu,%.4f,%d\n",maxima[k],n,pages,payload,alloc,alloc?100.0*payload/alloc:0.0,n);
    }
    fclose(csv); HF_DestroyFile("student_heap.db");
    printf("Inserted %d student rows; sequential scan returned %d rows. Wrote hf_space_results.csv\n",n,scanCount);
    return scanCount==n?0:2;
}
