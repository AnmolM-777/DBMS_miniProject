/* Reproducible PF buffer benchmark: LRU vs MRU over read/write mixes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pf.h"

#define FILE_NAME "pf_bench.db"
#define NPAGES 128
#define OPS 10000
#define CAPACITY 8

static void die(const char *what, int rc) { if (rc != PFE_OK) { fprintf(stderr, "%s failed: %d\n", what, rc); exit(1); } }
static void prepare(void) {
    int fd, p, i; char *buf;
    PF_DestroyFile(FILE_NAME); die("create", PF_CreateFile(FILE_NAME));
    fd=PF_OpenFile(FILE_NAME); if(fd<0) die("open",fd);
    for(i=0;i<NPAGES;i++){ die("alloc",PF_AllocPage(fd,&p,&buf)); memset(buf,0,PF_PAGE_SIZE); die("unfix",PF_UnfixPage(fd,p,1)); }
    die("close",PF_CloseFile(fd));
}
static void run(int policy, int writePct, FILE *out) {
    int fd, i, page, rc; char *buf; PF_Stats st; unsigned int seed=20261106u;
    die("config", PF_SetBufferConfig(CAPACITY,policy)); PF_ResetStats();
    fd=PF_OpenFile(FILE_NAME); if(fd<0) die("open",fd);
    for(i=0;i<OPS;i++) {
        seed=1664525u*seed+1013904223u; page=(int)((seed>>8)%NPAGES);
        rc=PF_GetThisPage(fd,page,&buf); die("get page",rc);
        if ((int)(i%100)<writePct) { buf[0]=(char)(buf[0]+1); die("unfix write",PF_UnfixPage(fd,page,1)); }
        else die("unfix read",PF_UnfixPage(fd,page,0));
    }
    die("close",PF_CloseFile(fd)); PF_GetStats(&st);
    fprintf(out,"%s,%d,%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",policy==PF_REPLACEMENT_LRU?"LRU":"MRU",writePct,OPS,
      st.logical_reads,st.logical_writes,st.physical_reads,st.physical_writes,st.page_hits,st.page_misses,st.evictions);
}
int main(void) {
    int p, policy; FILE *f;
    PF_Init(); die("initial config", PF_SetBufferConfig(CAPACITY,PF_REPLACEMENT_LRU)); prepare(); f=fopen("pf_buffer_results.csv","w"); if(!f){perror("csv");return 1;}
    fprintf(f,"policy,write_percent,operations,logical_reads,logical_writes,physical_reads,physical_writes,cache_hits,cache_misses,evictions\n");
    for(policy=PF_REPLACEMENT_LRU;policy<=PF_REPLACEMENT_MRU;policy++)
      for(p=0;p<=100;p+=25) run(policy,p,f);
    fclose(f); PF_DestroyFile(FILE_NAME); puts("Wrote pf_buffer_results.csv"); return 0;
}
