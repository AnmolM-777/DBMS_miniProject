#ifndef HF_H
#define HF_H

#include "pf.h"

/* Record ID structure pointing to a specific page and slot */
typedef struct {
    int pageNum;
    int slotNo;
} RID;

/* Slotted Page Header */
typedef struct {
    char pageType;       /* 'h' for heap page */
    short numSlots;      /* number of slots in the slot array */
    short freeSpaceOffset; /* byte offset where free space starts (growing downwards) */
} HF_PAGEHEADER;

/* Slot entry in the slot array */
typedef struct {
    short offset;        /* byte offset of the record on the page (-1 if free) */
    short length;        /* length of the record (0 if free) */
} HF_SLOT;

/* Error codes */
#define HFE_OK          0
#define HFE_PF          -1
#define HFE_EOF         -2
#define HFE_NOSPACE     -3
#define HFE_INVALIDSLOT -4
#define HFE_FD          -5

/* Function Prototypes */
int HF_CreateFile(char *fileName);
int HF_DestroyFile(char *fileName);
int HF_OpenFile(char *fileName);
int HF_CloseFile(int fileDesc);
int HF_InsertRec(int fileDesc, char *recData, int recLen, RID *rid);
int HF_DeleteRec(int fileDesc, RID *rid);
int HF_GetRec(int fileDesc, RID *rid, char *recData, int *recLen);
int HF_OpenScan(int fileDesc);
int HF_FindNextRec(int scanDesc, RID *rid, char *recData, int *recLen);
int HF_CloseScan(int scanDesc);
void HF_PrintError(char *s);

#endif
