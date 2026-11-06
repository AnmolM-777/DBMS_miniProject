/* pf.c: Paged File Interface routines */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pf.h"
#include "pftypes.h"

#define PF_MAX_OPEN_FILES 20
#define PF_PAGE_LIST_END -1
#define PF_PAGE_USED -2

int PFerrno = PFE_OK;
static PFftab_ele PFftab[PF_MAX_OPEN_FILES];
static int PFftabInit = 0;

extern int PFbufGet(int fd, int pagenum, PFfpage **fpage, int (*readfcn)(), int (*writefcn)());
extern int PFbufUnfix(int fd, int pagenum, int dirty);
extern int PFbufAlloc(int fd, int pagenum, PFfpage **fpage, int (*writefcn)());
extern int PFbufReleaseFile(int fd, int (*writefcn)());
extern int PFbufUsed(int fd, int pagenum);

static int PFreadfcn(int fd, int pagenum, PFfpage *fpage)
{
	off_t offset;
	ssize_t bytesRead;

	offset = (off_t)sizeof(PFhdr_str) + (off_t)pagenum * sizeof(PFfpage);
	if (lseek(PFftab[fd].unixfd, offset, SEEK_SET) < 0) {
		PFerrno = PFE_UNIX;
		return PFE_UNIX;
	}

	bytesRead = read(PFftab[fd].unixfd, fpage, sizeof(PFfpage));
	if (bytesRead != sizeof(PFfpage)) {
		PFerrno = PFE_INCOMPLETEREAD;
		return PFE_INCOMPLETEREAD;
	}

	return PFE_OK;
}

static int PFwritefcn(int fd, int pagenum, PFfpage *fpage)
{
	off_t offset;
	ssize_t bytesWritten;

	offset = (off_t)sizeof(PFhdr_str) + (off_t)pagenum * sizeof(PFfpage);
	if (lseek(PFftab[fd].unixfd, offset, SEEK_SET) < 0) {
		PFerrno = PFE_UNIX;
		return PFE_UNIX;
	}

	bytesWritten = write(PFftab[fd].unixfd, fpage, sizeof(PFfpage));
	if (bytesWritten != sizeof(PFfpage)) {
		PFerrno = PFE_INCOMPLETEWRITE;
		return PFE_INCOMPLETEWRITE;
	}

	return PFE_OK;
}

void PF_Init()
{
	int i;
	if (!PFftabInit) {
		for (i = 0; i < PF_MAX_OPEN_FILES; i++) {
			PFftab[i].fname = NULL;
		}
		PFhashInit();
		PFftabInit = 1;
	}
}

int PF_CreateFile(char *fname)
{
	int fd;
	PFhdr_str hdr;

	fd = open(fname, O_CREAT | O_EXCL | O_WRONLY, 0666);
	if (fd < 0) {
		PFerrno = PFE_UNIX;
		return PFE_UNIX;
	}

	hdr.firstfree = PF_PAGE_LIST_END;
	hdr.numpages = 0;

	if (write(fd, &hdr, sizeof(PFhdr_str)) != sizeof(PFhdr_str)) {
		close(fd);
		unlink(fname);
		PFerrno = PFE_HDRWRITE;
		return PFE_HDRWRITE;
	}

	close(fd);
	return PFE_OK;
}

int PF_DestroyFile(char *fname)
{
	int i;

	for (i = 0; i < PF_MAX_OPEN_FILES; i++) {
		if (PFftab[i].fname != NULL && strcmp(PFftab[i].fname, fname) == 0) {
			PFerrno = PFE_FILEOPEN;
			return PFE_FILEOPEN;
		}
	}

	if (unlink(fname) < 0) {
		PFerrno = PFE_UNIX;
		return PFE_UNIX;
	}

	return PFE_OK;
}

int PF_OpenFile(char *fname)
{
	int i, slot = -1;
	int fd;
	PFhdr_str hdr;

	for (i = 0; i < PF_MAX_OPEN_FILES; i++) {
		if (PFftab[i].fname == NULL) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		PFerrno = PFE_FTABFULL;
		return PFE_FTABFULL;
	}

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		PFerrno = PFE_UNIX;
		return PFE_UNIX;
	}

	if (read(fd, &hdr, sizeof(PFhdr_str)) != sizeof(PFhdr_str)) {
		close(fd);
		PFerrno = PFE_HDRREAD;
		return PFE_HDRREAD;
	}

	PFftab[slot].fname = strdup(fname);
	PFftab[slot].unixfd = fd;
	PFftab[slot].hdr = hdr;
	PFftab[slot].hdrchanged = 0;

	return slot;
}

int PF_OpenFileEx(char *fname, int buffer_capacity, int replacement_policy)
{
    int rc = PF_SetBufferConfig(buffer_capacity, replacement_policy);
    if (rc != PFE_OK) { PFerrno = PFE_NOBUF; return PFE_NOBUF; }
    return PF_OpenFile(fname);
}

int PF_CloseFile(int fd)
{
	int error;

	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	if ((error = PFbufReleaseFile(fd, PFwritefcn)) != PFE_OK)
		return error;

	if (PFftab[fd].hdrchanged) {
		if (lseek(PFftab[fd].unixfd, 0, SEEK_SET) < 0) {
			PFerrno = PFE_UNIX;
			return PFE_UNIX;
		}
		if (write(PFftab[fd].unixfd, &PFftab[fd].hdr, sizeof(PFhdr_str)) != sizeof(PFhdr_str)) {
			PFerrno = PFE_HDRWRITE;
			return PFE_HDRWRITE;
		}
	}

	close(PFftab[fd].unixfd);
	free(PFftab[fd].fname);
	PFftab[fd].fname = NULL;

	return PFE_OK;
}

int PF_GetFirstPage(int fd, int *pagenum, char **pagebuf)
{
	*pagenum = -1;
	return PF_GetNextPage(fd, pagenum, pagebuf);
}

int PF_GetNextPage(int fd, int *pagenum, char **pagebuf)
{
	int curPage;
	PFfpage *fpage;
	int error;

	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	curPage = *pagenum + 1;
	while (curPage < PFftab[fd].hdr.numpages) {
		if ((error = PFbufGet(fd, curPage, &fpage, PFreadfcn, PFwritefcn)) != PFE_OK)
			return error;
        PF_RecordLogicalRead();

		if (fpage->nextfree == PF_PAGE_USED) {
			*pagenum = curPage;
			*pagebuf = fpage->pagebuf;
			return PFE_OK;
		}

		if ((error = PFbufUnfix(fd, curPage, 0)) != PFE_OK)
			return error;

		curPage++;
	}

	PFerrno = PFE_EOF;
	return PFE_EOF;
}

int PF_GetThisPage(int fd, int pagenum, char **pagebuf)
{
	PFfpage *fpage;
	int error;

	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	if (pagenum < 0 || pagenum >= PFftab[fd].hdr.numpages) {
		PFerrno = PFE_INVALIDPAGE;
		return PFE_INVALIDPAGE;
	}

	if ((error = PFbufGet(fd, pagenum, &fpage, PFreadfcn, PFwritefcn)) != PFE_OK)
		return error;
    PF_RecordLogicalRead();

	if (fpage->nextfree != PF_PAGE_USED) {
		PFbufUnfix(fd, pagenum, 0);
		PFerrno = PFE_INVALIDPAGE;
		return PFE_INVALIDPAGE;
	}

	*pagebuf = fpage->pagebuf;
	return PFE_OK;
}

int PF_AllocPage(int fd, int *pagenum, char **pagebuf)
{
	int newPageNum;
	PFfpage *fpage;
	int error;

	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	if (PFftab[fd].hdr.firstfree != PF_PAGE_LIST_END) {
		newPageNum = PFftab[fd].hdr.firstfree;
		if ((error = PFbufGet(fd, newPageNum, &fpage, PFreadfcn, PFwritefcn)) != PFE_OK)
			return error;

		PFftab[fd].hdr.firstfree = fpage->nextfree;
		PFftab[fd].hdrchanged = 1;

		fpage->nextfree = PF_PAGE_USED;
		if ((error = PFbufUsed(fd, newPageNum)) != PFE_OK)
			return error;
	} else {
		newPageNum = PFftab[fd].hdr.numpages;
		if ((error = PFbufAlloc(fd, newPageNum, &fpage, PFwritefcn)) != PFE_OK)
			return error;

		PFftab[fd].hdr.numpages++;
		PFftab[fd].hdrchanged = 1;
		fpage->nextfree = PF_PAGE_USED;
	}

	*pagenum = newPageNum;
	*pagebuf = fpage->pagebuf;
	return PFE_OK;
}

int PF_DisposePage(int fd, int pagenum)
{
	PFfpage *fpage;
	int error;

	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	if (pagenum < 0 || pagenum >= PFftab[fd].hdr.numpages) {
		PFerrno = PFE_INVALIDPAGE;
		return PFE_INVALIDPAGE;
	}

	if ((error = PFbufGet(fd, pagenum, &fpage, PFreadfcn, PFwritefcn)) != PFE_OK)
		return error;

	if (fpage->nextfree != PF_PAGE_USED) {
		PFbufUnfix(fd, pagenum, 0);
		PFerrno = PFE_PAGEFREE;
		return PFE_PAGEFREE;
	}

	fpage->nextfree = PFftab[fd].hdr.firstfree;
	PFftab[fd].hdr.firstfree = pagenum;
	PFftab[fd].hdrchanged = 1;

	return PFbufUnfix(fd, pagenum, 1);
}

int PF_MarkDirty(int fd, int pagenum)
{
    if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) { PFerrno=PFE_FD; return PFE_FD; }
    return PFbufUsed(fd,pagenum);
}

int PF_UnfixPage(int fd, int pagenum, int dirty)
{
	if (fd < 0 || fd >= PF_MAX_OPEN_FILES || PFftab[fd].fname == NULL) {
		PFerrno = PFE_FD;
		return PFE_FD;
	}

	return PFbufUnfix(fd, pagenum, dirty);
}

void PF_PrintError(char *s)
{
	fprintf(stderr, "%s: error code %d\n", s, PFerrno);
}
