/* Student roll-number B+ tree experiments.
 * 1) Build from the already populated student file in one pass (file order).
 * 2) Start empty and insert entries in a deterministic shuffled order.
 * 3) True bottom-up bulk loading: sort keys, pack leaf pages, then build
 *    internal levels from child minima. Keys are the 8-byte second field.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pf.h"
#include "am.h"
#include "testam.h"
#define ROLL_LEN 8

typedef struct { char key[ROLL_LEN]; int rid; } Entry;
typedef struct { char key[ROLL_LEN]; int start, count; } Group;
typedef struct { int page; char minKey[ROLL_LEN]; } BulkNode;

static void die(const char *s,int rc){if(rc!=AME_OK && rc!=PFE_OK){fprintf(stderr,"%s failed: %d\n",s,rc);exit(1);}}
static int cmp_entry(const void *a,const void *b){return memcmp(((const Entry*)a)->key,((const Entry*)b)->key,ROLL_LEN);}
static Entry *load(const char *path,int *n){
    FILE*f=fopen(path,"r");char line[4096],*first,*second,*end;Entry*a=NULL;int cap=0;*n=0;
    if(!f){perror(path);exit(1);} if(!fgets(line,sizeof(line),f)){fclose(f);return NULL;}
    while(fgets(line,sizeof(line),f)){
        first=strchr(line,';'); if(!first)continue; second=strchr(first+1,';'); if(!second)continue;
        *second='\0'; end=first+1; if(strlen(end)!=ROLL_LEN){fprintf(stderr,"unexpected roll number width: %s\n",end);exit(1);}
        if(*n==cap){cap=cap?cap*2:1024;a=realloc(a,(size_t)cap*sizeof(*a));if(!a)exit(1);}
        memcpy(a[*n].key,end,ROLL_LEN);a[*n].rid=*n;(*n)++;
    }
    fclose(f);return a;
}
static int key_has_rid(const Entry *a,int n,const char *key,int rid){int i;for(i=0;i<n;i++)if(!memcmp(a[i].key,key,ROLL_LEN)&&a[i].rid==rid)return 1;return 0;}
static void query_and_report(int fd,const char *label,Entry*a,int n,FILE*out,int mode){
    int i,sd,rc,correct=0;PF_Stats st;clock_t t0=clock(),t1;
    PF_ResetStats();t0=clock();
    for(i=0;i<100;i++){
        int ix=(i*7919)%n;
        sd=AM_OpenIndexScan(fd,CHAR_TYPE,ROLL_LEN,EQ_OP,a[ix].key);
        if(sd<0){fprintf(stderr,"%s scan open failed: %d\n",label,sd);exit(1);}
        rc=AM_FindNextEntry(sd);
        if(rc>=0 && key_has_rid(a,n,a[ix].key,rc))correct++; else { int pg,ixpos,ss,raw=-999; char *pb; AM_LEAFHEADER lh; short ptr=0; ss=AM_Search(fd,CHAR_TYPE,ROLL_LEN,a[ix].key,&pg,&pb,&ixpos); if(ss>=0){memcpy(&lh,pb,AM_sl);if(ixpos>0&&ixpos<=lh.numKeys){memcpy(&ptr,pb+AM_sl+(ixpos-1)*(ROLL_LEN+AM_ss)+ROLL_LEN,AM_ss);if(ptr>0)memcpy(&raw,pb+ptr,sizeof(int));}PF_UnfixPage(fd,pg,FALSE);} fprintf(stderr,"lookup mismatch mode %d key=%.*s got=%d expected=%d search=%d page=%d slot=%d raw=%d\n",mode,ROLL_LEN,a[ix].key,rc,a[ix].rid,ss,pg,ixpos,raw); }
        AM_CloseIndexScan(sd);
    }
    t1=clock();PF_GetStats(&st);
    fprintf(out,"%s_lookup,100,%d,%.6f,%lu,%lu,%lu,%d\n",label,mode,(double)(t1-t0)/CLOCKS_PER_SEC,st.logical_reads,st.physical_reads,st.physical_writes,correct);
}
static void build_insert(const char *name,Entry*a,int n,int mode,FILE*out){
    int i,fd,rc;char fname[128];clock_t t0,t1;PF_Stats st;double sec;
    PF_ResetStats();t0=clock();AM_DestroyIndex((char*)name,0);
    die("create index",AM_CreateIndex((char*)name,0,CHAR_TYPE,ROLL_LEN));
    snprintf(fname,sizeof(fname),"%s.0",name);fd=PF_OpenFile(fname);if(fd<0)exit(1);
    for(i=0;i<n;i++){rc=AM_InsertEntry(fd,CHAR_TYPE,ROLL_LEN,a[i].key,a[i].rid);if(rc!=AME_OK){fprintf(stderr,"insert mode %d row %d failed %d\n",mode,i,rc);exit(1);}}
    die("close build index",PF_CloseFile(fd));t1=clock();PF_GetStats(&st);sec=(double)(t1-t0)/CLOCKS_PER_SEC;
    fprintf(out,"%s,%d,%d,%.6f,%lu,%lu,%lu,0\n",mode==0?"file_order_build":"incremental_shuffled_build",n,mode,sec,st.logical_reads,st.physical_reads,st.physical_writes);
    fd=PF_OpenFile(fname);if(fd<0)exit(1);query_and_report(fd,mode==0?"file_order":"incremental_shuffled",a,n,out,mode);
    { int scanCount=0,scanRid,sd=AM_OpenIndexScan(fd,CHAR_TYPE,ROLL_LEN,0,NULL); if(sd<0)exit(1); while((scanRid=AM_FindNextEntry(sd))>=0)scanCount++; AM_CloseIndexScan(sd); if(scanCount!=n){fprintf(stderr,"%s full scan returned %d records, expected %d\n",name,scanCount,n);exit(1);} }
    die("close query index",PF_CloseFile(fd));die("destroy index",AM_DestroyIndex((char*)name,0));
}

static void fill_internal(char *buf,BulkNode *children,int count,int maxKeys){
    AM_INTHEADER h;int i,recSize=sizeof(int)+ROLL_LEN;h.pageType='i';h.attrLength=ROLL_LEN;h.numKeys=(short)(count-1);h.maxKeys=(short)maxKeys;
    memcpy(buf,&h,AM_sint);memcpy(buf+AM_sint,&children[0].page,sizeof(int));
    for(i=1;i<count;i++){int off=AM_sint+sizeof(int)+(i-1)*recSize;memcpy(buf+off,children[i].minKey,ROLL_LEN);memcpy(buf+off+ROLL_LEN,&children[i].page,sizeof(int));}
}
static void bulk_load(const char *name,Entry *sorted,int n,FILE*out){
    char fname[128],*buf;int fd,page,rootPage,i,j,unique=0,maxKeys,maxChildren,leafCount=0,groupAt=0,rc;
    Group *groups;BulkNode *level,*next;clock_t t0,t1;PF_Stats st;double sec;
    /* Collapse equal roll numbers into one B+ tree key with a RID chain. */
    groups=calloc((size_t)n,sizeof(*groups));if(!groups)exit(1);
    for(i=0;i<n;){j=i+1;while(j<n&&!memcmp(sorted[i].key,sorted[j].key,ROLL_LEN))j++;memcpy(groups[unique].key,sorted[i].key,ROLL_LEN);groups[unique].start=i;groups[unique].count=j-i;unique++;i=j;}
    PF_ResetStats();t0=clock();snprintf(fname,sizeof(fname),"%s.0",name);PF_DestroyFile(fname);rc=PF_CreateFile(fname);if(rc!=PFE_OK)die("bulk create",rc);
    fd=PF_OpenFile(fname);if(fd<0)exit(1);
    if(PF_AllocPage(fd,&rootPage,&buf)!=PFE_OK||rootPage!=0) exit(1);
    memset(buf,0,PF_PAGE_SIZE);
    PF_UnfixPage(fd,rootPage,TRUE);
    /* The legacy AM scan helper expects the first leaf at page 2. */
    if(PF_AllocPage(fd,&page,&buf)!=PFE_OK) exit(1);
    memset(buf,0,PF_PAGE_SIZE);
    buf[0]='x';
    PF_UnfixPage(fd,page,TRUE);
    maxKeys=(PF_PAGE_SIZE-(int)AM_sint-(int)sizeof(int))/(sizeof(int)+ROLL_LEN);if(maxKeys%2)maxKeys--;maxChildren=maxKeys+1;
    /* Greedily pack variable-size duplicate RID chains into leaf pages. */
    {int g=0;while(g<unique){int used=AM_sl,keys=0;while(g<unique&&keys<maxKeys){int need=ROLL_LEN+sizeof(short)+groups[g].count*(sizeof(int)+sizeof(short));if(keys&&used+need>PF_PAGE_SIZE)break;if(used+need>PF_PAGE_SIZE){fprintf(stderr,"one roll-number group does not fit a page\n");exit(1);}used+=need;keys++;g++;}leafCount++;}}
    level=calloc((size_t)leafCount,sizeof(*level));if(!level)exit(1);
    /* Allocate leaves first so their next-leaf pointers are known. */
    for(i=0;i<leafCount;i++){if(PF_AllocPage(fd,&page,&buf)!=PFE_OK)exit(1);level[i].page=page;memcpy(level[i].minKey,groups[groupAt].key,ROLL_LEN);{int used=AM_sl,keys=0;while(groupAt<unique&&keys<maxKeys){int need=ROLL_LEN+sizeof(short)+groups[groupAt].count*(sizeof(int)+sizeof(short));if(keys&&used+need>PF_PAGE_SIZE)break;used+=need;keys++;groupAt++;}}PF_UnfixPage(fd,page,TRUE);}
    groupAt=0;
    for(i=0;i<leafCount;i++){
        int startGroup=groupAt,keys=0,ridTotal=0,used=AM_sl,entryIndex=0;AM_LEAFHEADER h;
        while(groupAt<unique&&keys<maxKeys){int need=ROLL_LEN+sizeof(short)+groups[groupAt].count*(sizeof(int)+sizeof(short));if(keys&&used+need>PF_PAGE_SIZE)break;used+=need;ridTotal+=groups[groupAt].count;keys++;groupAt++;}
        if(PF_GetThisPage(fd,level[i].page,&buf)!=PFE_OK) exit(1);
        memset(buf,0,PF_PAGE_SIZE);
        h.pageType='l';h.nextLeafPage=(i+1<leafCount)?level[i+1].page:AM_NULL_PAGE;h.recIdPtr=(short)(PF_PAGE_SIZE-ridTotal*(sizeof(int)+sizeof(short)));h.keyPtr=(short)(AM_sl+keys*(ROLL_LEN+sizeof(short)));h.freeListPtr=0;h.numinfreeList=0;h.attrLength=ROLL_LEN;h.numKeys=(short)keys;h.maxKeys=(short)maxKeys;memcpy(buf,&h,AM_sl);
        {int ridOrdinal=0,g;for(g=startGroup;g<groupAt;g++,entryIndex++){
            short firstPtr=(short)(PF_PAGE_SIZE-(ridOrdinal+1)*(sizeof(int)+sizeof(short)));
            memcpy(buf+AM_sl+entryIndex*(ROLL_LEN+sizeof(short)),groups[g].key,ROLL_LEN);memcpy(buf+AM_sl+entryIndex*(ROLL_LEN+sizeof(short))+ROLL_LEN,&firstPtr,sizeof(short));
            for(j=0;j<groups[g].count;j++){short ptr=(short)(PF_PAGE_SIZE-(ridOrdinal+1)*(sizeof(int)+sizeof(short)));short nextPtr=(j+1<groups[g].count)?(short)(PF_PAGE_SIZE-(ridOrdinal+2)*(sizeof(int)+sizeof(short))):0;int rid=sorted[groups[g].start+j].rid;memcpy(buf+ptr,&rid,sizeof(int));memcpy(buf+ptr+sizeof(int),&nextPtr,sizeof(short));ridOrdinal++;}
        }}
        PF_UnfixPage(fd,level[i].page,TRUE);
    }
    /* Build internal levels bottom-up; root must remain page zero. */
    while(leafCount>maxChildren){int parentCount=(leafCount+maxChildren-1)/maxChildren;next=calloc((size_t)parentCount,sizeof(*next));if(!next)exit(1);for(i=0;i<parentCount;i++){int start=i*maxChildren,count=leafCount-start;if(count>maxChildren)count=maxChildren;if(PF_AllocPage(fd,&page,&buf)!=PFE_OK)exit(1);next[i].page=page;memcpy(next[i].minKey,level[start].minKey,ROLL_LEN);fill_internal(buf,&level[start],count,maxKeys);PF_UnfixPage(fd,page,TRUE);}free(level);level=next;leafCount=parentCount;}
    if(PF_GetThisPage(fd,0,&buf)!=PFE_OK) exit(1);
    memset(buf,0,PF_PAGE_SIZE);
    fill_internal(buf,level,leafCount,maxKeys);
    PF_UnfixPage(fd,0,TRUE);
    AM_RootPageNum=0;AM_LeftPageNum=2;free(level);free(groups);die("close bulk index",PF_CloseFile(fd));t1=clock();PF_GetStats(&st);sec=(double)(t1-t0)/CLOCKS_PER_SEC;
    fprintf(out,"bottom_up_bulk_load,%d,3,%.6f,%lu,%lu,%lu,0\n",n,sec,st.logical_reads,st.physical_reads,st.physical_writes);
}
int main(int argc,char**argv){
    const char*path=argc>1?argv[1]:"../data/student.txt";int n,i;Entry*base=load(path,&n),*shuf;FILE*out;unsigned int seed=42;
    if(!base||n==0) return 1;
    shuf=malloc((size_t)n*sizeof(*shuf));
    if(!shuf) return 1;
    memcpy(shuf,base,(size_t)n*sizeof(*base));
    for(i=n-1;i>0;i--){int j;Entry tmp;seed=1664525u*seed+1013904223u;j=(int)(seed%(unsigned)(i+1));tmp=shuf[i];shuf[i]=shuf[j];shuf[j]=tmp;}
    PF_Init();out=fopen("am_index_results.csv","w");if(!out)return 1;fprintf(out,"experiment,records,mode,seconds,logical_page_accesses,physical_reads,physical_writes,correct_point_queries\n");
    build_insert("student_file_order",base,n,0,out);build_insert("student_incremental",shuf,n,1,out);
    qsort(base,(size_t)n,sizeof(*base),cmp_entry);bulk_load("student_sorted_load",base,n,out);
    {char fn[128];int fd,i,correct=0,sd,scanCount=0,scanRid;PF_Stats st;clock_t t0,t1;snprintf(fn,sizeof(fn),"%s.0","student_sorted_load");fd=PF_OpenFile(fn);if(fd<0)exit(1);PF_ResetStats();t0=clock();for(i=0;i<100;i++){int ix=(i*7919)%n;sd=AM_OpenIndexScan(fd,CHAR_TYPE,ROLL_LEN,EQ_OP,base[ix].key);if(sd<0)exit(1);scanRid=AM_FindNextEntry(sd);if(scanRid>=0&&key_has_rid(base,n,base[ix].key,scanRid))correct++;AM_CloseIndexScan(sd);}t1=clock();PF_GetStats(&st);fprintf(out,"bottom_up_bulk_load_lookup,100,3,%.6f,%lu,%lu,%lu,%d\n",(double)(t1-t0)/CLOCKS_PER_SEC,st.logical_reads,st.physical_reads,st.physical_writes,correct);sd=AM_OpenIndexScan(fd,CHAR_TYPE,ROLL_LEN,0,NULL);if(sd<0)exit(1);while((scanRid=AM_FindNextEntry(sd))>=0)scanCount++;AM_CloseIndexScan(sd);if(scanCount!=n){fprintf(stderr,"bulk scan count %d expected %d\n",scanCount,n);exit(1);}PF_CloseFile(fd);AM_DestroyIndex("student_sorted_load",0);}
    fclose(out);free(base);free(shuf);printf("Indexed %d student records using the 8-byte roll-number field; wrote am_index_results.csv\n",n);return 0;
}
