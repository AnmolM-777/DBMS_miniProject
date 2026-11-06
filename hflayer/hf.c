#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hf.h"
#include "pf.h"

#define HF_MAXSCANS 20
#define HF_SL (sizeof(HF_PAGEHEADER))
#define HF_SS (sizeof(HF_SLOT))

static int HF_Errno = HFE_OK;

/* Pack live records at the end of the page and update their slot offsets. */
static void HF_CompactPage(char *pageBuf, HF_PAGEHEADER *header)
{
    char packed[PF_PAGE_SIZE];
    int i, offset = PF_PAGE_SIZE;
    HF_SLOT slot;
    memset(packed, 0, sizeof(packed));
    for (i = 0; i < header->numSlots; ++i) {
        memcpy(&slot, pageBuf + HF_SL + i * HF_SS, HF_SS);
        if (slot.offset >= 0 && slot.length > 0) {
            offset -= slot.length;
            memcpy(packed + offset, pageBuf + slot.offset, slot.length);
            slot.offset = (short)offset;
            memcpy(pageBuf + HF_SL + i * HF_SS, &slot, HF_SS);
        }
    }
    memcpy(pageBuf + offset, packed + offset, PF_PAGE_SIZE - offset);
    header->freeSpaceOffset = (short)offset;
    memcpy(pageBuf, header, HF_SL);
}


static struct {
    int fileDesc;
    int pageNum;
    int slotNo;
    int status;
} HF_ScanTable[HF_MAXSCANS];

int HF_CreateFile(char *fileName) {
    return PF_CreateFile(fileName);
}

int HF_DestroyFile(char *fileName) {
    return PF_DestroyFile(fileName);
}

int HF_OpenFile(char *fileName) {
    return PF_OpenFile(fileName);
}

int HF_CloseFile(int fileDesc) {
    return PF_CloseFile(fileDesc);
}

int HF_InsertRec(int fileDesc, char *recData, int recLen, RID *rid) {
    char *pageBuf = NULL;
    int pageNum = 0, errVal, foundPage = 0, slotNo = -1, i;
    HF_PAGEHEADER header;
    HF_SLOT slot;
    int freeEnd, needed;
    if (fileDesc < 0 || !recData || !rid || recLen <= 0 || recLen > PF_PAGE_SIZE - HF_SL - HF_SS) return HFE_FD;

    errVal = PF_GetFirstPage(fileDesc, &pageNum, &pageBuf);
    while (errVal == PFE_OK) {
        memcpy(&header, pageBuf, HF_SL);
        slotNo = -1;
        for (i = 0; i < header.numSlots; ++i) {
            memcpy(&slot, pageBuf + HF_SL + i * HF_SS, HF_SS);
            if (slot.offset < 0 || slot.length == 0) { slotNo = i; break; }
        }
        HF_CompactPage(pageBuf, &header);
        freeEnd = HF_SL + (header.numSlots + (slotNo < 0 ? 1 : 0)) * HF_SS;
        needed = recLen + (slotNo < 0 ? HF_SS : 0);
        if ((int)header.freeSpaceOffset - freeEnd >= needed) { foundPage = 1; break; }
        PF_UnfixPage(fileDesc, pageNum, FALSE);
        errVal = PF_GetNextPage(fileDesc, &pageNum, &pageBuf);
    }
    if (!foundPage) {
        errVal = PF_AllocPage(fileDesc, &pageNum, &pageBuf);
        if (errVal != PFE_OK) { HF_Errno = HFE_PF; return HFE_PF; }
        memset(pageBuf, 0, PF_PAGE_SIZE);
        header.pageType = 'h'; header.numSlots = 0; header.freeSpaceOffset = PF_PAGE_SIZE;
        slotNo = -1;
    }
    if (slotNo < 0) slotNo = header.numSlots++;
    header.freeSpaceOffset = (short)(header.freeSpaceOffset - recLen);
    memcpy(pageBuf + header.freeSpaceOffset, recData, (size_t)recLen);
    slot.offset = header.freeSpaceOffset; slot.length = (short)recLen;
    memcpy(pageBuf + HF_SL + slotNo * HF_SS, &slot, HF_SS);
    memcpy(pageBuf, &header, HF_SL);
    errVal = PF_UnfixPage(fileDesc, pageNum, TRUE);
    if (errVal != PFE_OK) { HF_Errno = HFE_PF; return HFE_PF; }
    rid->pageNum = pageNum; rid->slotNo = slotNo;
    return HFE_OK;
}

int HF_GetRec(int fileDesc, RID *rid, char *recData, int *recLen) {
    char *pageBuf;
    int errVal;
    HF_PAGEHEADER header;
    HF_SLOT slot;

    if (fileDesc < 0) return HFE_FD;

    errVal = PF_GetThisPage(fileDesc, rid->pageNum, &pageBuf);
    if (errVal != PFE_OK) {
        HF_Errno = HFE_PF;
        return HFE_PF;
    }

    memcpy(&header, pageBuf, HF_SL);
    if (rid->slotNo < 0 || rid->slotNo >= header.numSlots) {
        PF_UnfixPage(fileDesc, rid->pageNum, FALSE);
        return HFE_INVALIDSLOT;
    }

    memcpy(&slot, pageBuf + HF_SL + rid->slotNo * HF_SS, HF_SS);
    if (slot.offset == -1 || slot.length == 0) {
        PF_UnfixPage(fileDesc, rid->pageNum, FALSE);
        return HFE_INVALIDSLOT;
    }

    memcpy(recData, pageBuf + slot.offset, slot.length);
    *recLen = slot.length;

    PF_UnfixPage(fileDesc, rid->pageNum, FALSE);
    return HFE_OK;
}

int HF_DeleteRec(int fileDesc, RID *rid) {
    char *pageBuf;
    int errVal;
    HF_PAGEHEADER header;
    HF_SLOT slot;

    if (fileDesc < 0) return HFE_FD;

    errVal = PF_GetThisPage(fileDesc, rid->pageNum, &pageBuf);
    if (errVal != PFE_OK) return HFE_PF;

    memcpy(&header, pageBuf, HF_SL);
    if (rid->slotNo < 0 || rid->slotNo >= header.numSlots) {
        PF_UnfixPage(fileDesc, rid->pageNum, FALSE);
        return HFE_INVALIDSLOT;
    }

    slot.offset = -1;
    slot.length = 0;
    memcpy(pageBuf + HF_SL + rid->slotNo * HF_SS, &slot, HF_SS);

    PF_UnfixPage(fileDesc, rid->pageNum, TRUE);
    return HFE_OK;
}

int HF_OpenScan(int fileDesc) {
    int scanDesc;
    for (scanDesc = 0; scanDesc < HF_MAXSCANS; scanDesc++) {
        if (HF_ScanTable[scanDesc].status == 0) break;
    }
    if (scanDesc >= HF_MAXSCANS) return HFE_PF;

    HF_ScanTable[scanDesc].fileDesc = fileDesc;
    HF_ScanTable[scanDesc].pageNum = 0;
    HF_ScanTable[scanDesc].slotNo = 0;
    HF_ScanTable[scanDesc].status = 1;

    return scanDesc;
}

int HF_FindNextRec(int scanDesc, RID *rid, char *recData, int *recLen) {
    char *pageBuf;
    int errVal;
    HF_PAGEHEADER header;
    HF_SLOT slot;

    if (scanDesc < 0 || scanDesc >= HF_MAXSCANS || HF_ScanTable[scanDesc].status == 0)
        return HFE_INVALIDSLOT;

    int fd = HF_ScanTable[scanDesc].fileDesc;
    int pageNum = HF_ScanTable[scanDesc].pageNum;
    int slotNo = HF_ScanTable[scanDesc].slotNo;

    if (pageNum == 0) {
        errVal = PF_GetFirstPage(fd, &pageNum, &pageBuf);
    } else {
        errVal = PF_GetThisPage(fd, pageNum, &pageBuf);
    }

    while (errVal == PFE_OK) {
        memcpy(&header, pageBuf, HF_SL);

        while (slotNo < header.numSlots) {
            memcpy(&slot, pageBuf + HF_SL + slotNo * HF_SS, HF_SS);
            if (slot.offset != -1 && slot.length > 0) {
                memcpy(recData, pageBuf + slot.offset, slot.length);
                *recLen = slot.length;
                rid->pageNum = pageNum;
                rid->slotNo = slotNo;

                HF_ScanTable[scanDesc].pageNum = pageNum;
                HF_ScanTable[scanDesc].slotNo = slotNo + 1;

                PF_UnfixPage(fd, pageNum, FALSE);
                return HFE_OK;
            }
            slotNo++;
        }

        PF_UnfixPage(fd, pageNum, FALSE);
        slotNo = 0;
        errVal = PF_GetNextPage(fd, &pageNum, &pageBuf);
    }

    return HFE_EOF;
}

int HF_CloseScan(int scanDesc) {
    if (scanDesc < 0 || scanDesc >= HF_MAXSCANS) return HFE_INVALIDSLOT;
    HF_ScanTable[scanDesc].status = 0;
    return HFE_OK;
}

void HF_PrintError(char *s) {
    fprintf(stderr, "%s error code %d\n", s, HF_Errno);
}
