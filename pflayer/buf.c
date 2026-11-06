/* buf.c: buffer management routines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pf.h"
#include "pftypes.h"

static int PFnumbpage = 0;
static int PFbufferCapacity = PF_DEFAULT_MAX_BUFS;
static int PFreplacementPolicy = PF_REPLACEMENT_LRU;
static PF_Stats PFstats;

int PF_SetBufferConfig(int capacity, int replacement_policy)
{
    if (capacity <= 0 || (replacement_policy != PF_REPLACEMENT_LRU && replacement_policy != PF_REPLACEMENT_MRU)) return -1;
    if (PFnumbpage > capacity) return -1;
    PFbufferCapacity = capacity;
    PFreplacementPolicy = replacement_policy;
    return PFE_OK;
}
void PF_RecordLogicalRead(void) { PFstats.logical_reads++; }
void PF_ResetStats(void) { memset(&PFstats, 0, sizeof(PFstats)); }
void PF_GetStats(PF_Stats *stats) { if (stats) *stats = PFstats; }
void PF_PrintStats(void) {
    printf("logical_reads=%lu logical_writes=%lu physical_reads=%lu physical_writes=%lu hits=%lu misses=%lu evictions=%lu\n",
      PFstats.logical_reads, PFstats.logical_writes, PFstats.physical_reads, PFstats.physical_writes,
      PFstats.page_hits, PFstats.page_misses, PFstats.evictions);
}
static PFbpage *PFfirstbpage= NULL;
static PFbpage *PFlastbpage = NULL;
static PFbpage *PFfreebpage= NULL;

static void PFbufInsertFree(PFbpage *bpage)
{
	bpage->nextpage = PFfreebpage;
	PFfreebpage = bpage;
}

static void PFbufLinkHead(PFbpage *bpage)
{
	bpage->nextpage = PFfirstbpage;
	bpage->prevpage = NULL;
	if (PFfirstbpage != NULL)
		PFfirstbpage->prevpage = bpage;
	PFfirstbpage = bpage;
	if (PFlastbpage == NULL)
		PFlastbpage = bpage;
}

void PFbufUnlink(PFbpage *bpage)
{
	if (PFfirstbpage == bpage)
		PFfirstbpage = bpage->nextpage;
	
	if (PFlastbpage == bpage)
		PFlastbpage = bpage->prevpage;
	
	if (bpage->nextpage != NULL)
		bpage->nextpage->prevpage = bpage->prevpage;
	
	if (bpage->prevpage != NULL)
		bpage->prevpage->nextpage = bpage->nextpage;

	bpage->prevpage = bpage->nextpage = NULL;
}

static int PFbufInternalAlloc(PFbpage **bpage, int (*writefcn)())
{
	PFbpage *tbpage;
	int error;

	if (PFfreebpage != NULL){
		*bpage = PFfreebpage;
		PFfreebpage = (*bpage)->nextpage;
	}
	else if (PFnumbpage < PFbufferCapacity){
		if ((*bpage=(PFbpage *)malloc(sizeof(PFbpage)))==NULL){
			*bpage = NULL;
			PFerrno = PFE_NOMEM;
			return(PFerrno);
		}
		PFnumbpage++;
	}
	else {
		*bpage = NULL;
		for (tbpage=(PFreplacementPolicy == PF_REPLACEMENT_MRU ? PFfirstbpage : PFlastbpage);tbpage!=NULL;tbpage=(PFreplacementPolicy == PF_REPLACEMENT_MRU ? tbpage->nextpage : tbpage->prevpage)){
			if (!tbpage->fixed)
				break;
		}

		if (tbpage == NULL){
			PFerrno = PFE_NOBUF;
			return(PFerrno);
		}

		if (tbpage->dirty&&((error=(*writefcn)(tbpage->fd,
				tbpage->page,&tbpage->fpage))!= PFE_OK))
			return(error);
        if (tbpage->dirty) PFstats.physical_writes++;
		tbpage->dirty = FALSE;
        PFstats.evictions++;

		if ((error=PFhashDelete(tbpage->fd,tbpage->page))!= PFE_OK)
			return(error);
		
		PFbufUnlink(tbpage);
		*bpage = tbpage;
	}

    /* A frame may be freshly malloc'ed or recycled from the free list. */
    (*bpage)->fixed = FALSE;
    (*bpage)->dirty = FALSE;
    (*bpage)->prevpage = NULL;
    (*bpage)->nextpage = NULL;
	PFbufLinkHead(*bpage);
	return(PFE_OK);
}

int PFbufGet(int fd, int pagenum, PFfpage **fpage, int (*readfcn)(), int (*writefcn)())
{
	PFbpage *bpage;
	int error;

	if ((bpage=PFhashFind(fd,pagenum)) == NULL){
        PFstats.page_misses++;
        PFstats.physical_reads++;
		if ((error=PFbufInternalAlloc(&bpage,writefcn))!= PFE_OK){
			*fpage = NULL;
			return(error);
		}
		
		if ((error=(*readfcn)(fd,pagenum,&bpage->fpage))!= PFE_OK){
			PFbufUnlink(bpage);
			PFbufInsertFree(bpage);
			*fpage = NULL;
			return(error);
		}

		if ((error=PFhashInsert(fd,pagenum,bpage))!=PFE_OK){
			PFbufUnlink(bpage);
			PFbufInsertFree(bpage);
			return(error);
		}

		bpage->fd = fd;
		bpage->page = pagenum;
		bpage->dirty = FALSE;
	}
	else { PFstats.page_hits++; }
	if (bpage != NULL && bpage->fixed){
		*fpage = &bpage->fpage;
		PFerrno = PFE_PAGEFIXED;
		return(PFerrno);
	}

	bpage->fixed = TRUE;
	*fpage = &bpage->fpage;
	return(PFE_OK);
}

int PFbufUnfix(int fd, int pagenum, int dirty)
{
	PFbpage *bpage;

	if ((bpage= PFhashFind(fd,pagenum))==NULL){
		PFerrno = PFE_PAGENOTINBUF;
		return(PFerrno);
	}

	if (!bpage->fixed){
		PFerrno = PFE_PAGEUNFIXED;
		return(PFerrno);
	}

	if (dirty) {
		bpage->dirty = TRUE;
        PFstats.logical_writes++;
    }
	
	bpage->fixed = FALSE;
	PFbufUnlink(bpage);
	PFbufLinkHead(bpage);
	return(PFE_OK);
}

int PFbufAlloc(int fd, int pagenum, PFfpage **fpage, int (*writefcn)())
{
	PFbpage *bpage;
	int error;

	*fpage = NULL;

	if ((bpage=PFhashFind(fd,pagenum))!= NULL){
		PFerrno = PFE_PAGEINBUF;
		return(PFerrno);
	}

	if ((error=PFbufInternalAlloc(&bpage,writefcn))!= PFE_OK)
		return(error);
	
	if ((error=PFhashInsert(fd,pagenum,bpage))!= PFE_OK){
		PFbufUnlink(bpage);
		PFbufInsertFree(bpage);
		return(error);
	}

	bpage->fd = fd;
	bpage->page = pagenum;
	bpage->fixed = TRUE;
	bpage->dirty = FALSE;

	*fpage = &bpage->fpage;
	return(PFE_OK);
}

int PFbufReleaseFile(int fd, int (*writefcn)())
{
	PFbpage *bpage;
	PFbpage *temppage;
	int error;

	bpage = PFfirstbpage;
	while (bpage != NULL){
		if (bpage->fd == fd){
			if (bpage->fixed){
				PFerrno = PFE_PAGEFIXED;
				return(PFerrno);
			}

			if (bpage->dirty&&((error=(*writefcn)(fd,bpage->page,
					&bpage->fpage))!= PFE_OK))
				return(error);
            if (bpage->dirty) PFstats.physical_writes++;
			bpage->dirty = FALSE;

			if ((error=PFhashDelete(fd,bpage->page))!= PFE_OK){
				printf("Internal error:PFbufReleaseFile()\n");
				exit(1);
			}

			temppage = bpage;
			bpage = bpage->nextpage;
			PFbufUnlink(temppage);
			PFbufInsertFree(temppage);
		}
		else bpage = bpage->nextpage;
	}
	return(PFE_OK);
}

int PFbufUsed(int fd, int pagenum)
{
	PFbpage *bpage;

	if ((bpage=PFhashFind(fd,pagenum))==NULL){
		PFerrno = PFE_PAGENOTINBUF;
		return(PFerrno);
	}

	if (!(bpage->fixed)){
		PFerrno = PFE_PAGEUNFIXED;
		return(PFerrno);
	}

	bpage->dirty = TRUE;
    PFstats.logical_writes++;
	PFbufUnlink(bpage);
	PFbufLinkHead(bpage);
	return(PFE_OK);
}

void PFbufPrint()
{
	PFbpage *bpage;

	printf("buffer content:\n");
	if (PFfirstbpage == NULL)
		printf("empty\n");
	else {
		printf("fd\tpage\tfixed\tdirty\tfpage\n");
		for(bpage = PFfirstbpage; bpage != NULL; bpage= bpage->nextpage)
			printf("%d\t%d\t%d\t%d\t%p\n",
				bpage->fd,bpage->page,(int)bpage->fixed,
				(int)bpage->dirty,(void *)&bpage->fpage);
	}
}
