#ifndef PFTYPES_H
#define PFTYPES_H

#include "pf.h"

#define PF_DEFAULT_MAX_BUFS 20
#define PF_HASH_SIZE 20

typedef struct PFhdr_str {
	int	firstfree;	/* first free page */
	int	numpages;	/* # of pages in file */
} PFhdr_str;

typedef struct PFfpage {
	int nextfree;
	char pagebuf[PF_PAGE_SIZE];
} PFfpage;

typedef struct PFbpage {
	int fd;
	int page;
	short fixed;
	short dirty;
	PFfpage fpage;
	struct PFbpage *nextpage;
	struct PFbpage *prevpage;
} PFbpage;

typedef struct PFftab_ele {
	char *fname;
	int unixfd;
	PFhdr_str hdr;
	short hdrchanged;
} PFftab_ele;

extern PFbpage *PFhashFind(int fd, int page);
extern int PFhashInsert(int fd, int page, PFbpage *bpage);
extern int PFhashDelete(int fd, int page);
extern void PFhashInit();
extern void PFhashPrint();

#endif
