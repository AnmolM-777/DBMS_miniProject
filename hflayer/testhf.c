#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pf.h"
#include "hf.h"
#define RELNAME "student_hf_test.db"
typedef struct { int roll_no; char name[30]; float cgpa; } Student;
#define CHECK(expr) do { int _r=(expr); if(_r!=HFE_OK){fprintf(stderr,"%s failed: %d\n",#expr,_r);return 1;} } while(0)
int main(void) {
    int fd, scan, len, count=0; RID r1,r2,r3; char buf[256]; Student s1={101,"Alice Smith",3.85f},s2={102,"Bob Johnson",3.42f},s3={103,"Charlie Brown",3.91f};
    PF_Init(); HF_DestroyFile(RELNAME);
    CHECK(HF_CreateFile(RELNAME)); fd=HF_OpenFile(RELNAME); if(fd<0)return 1;
    CHECK(HF_InsertRec(fd,(char*)&s1,sizeof(s1),&r1));
    CHECK(HF_InsertRec(fd,(char*)&s2,sizeof(s2),&r2));
    CHECK(HF_GetRec(fd,&r1,buf,&len)); if(len!=(int)sizeof(s1)||memcmp(buf,&s1,sizeof(s1)))return 2;
    CHECK(HF_DeleteRec(fd,&r2));
    if(HF_GetRec(fd,&r2,buf,&len)!=HFE_INVALIDSLOT)return 3;
    /* This insertion triggers compaction and reuses the deleted slot. */
    CHECK(HF_InsertRec(fd,(char*)&s3,sizeof(s3),&r3));
    scan=HF_OpenScan(fd); if(scan<0)return 4;
    while(HF_FindNextRec(scan,&r1,buf,&len)==HFE_OK)count++;
    CHECK(HF_CloseScan(scan)); if(count!=2){fprintf(stderr,"expected 2 live records, found %d\n",count);return 5;}
    CHECK(HF_CloseFile(fd)); CHECK(HF_DestroyFile(RELNAME));
    puts("HF insert/get/delete/slot-reuse/sequential-scan test passed"); return 0;
}
