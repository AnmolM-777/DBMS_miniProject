/* hash.c: hash table management routines */
#include <stdio.h>
#include <stdlib.h>
#include "pf.h"
#include "pftypes.h"

typedef struct PFhashentry {
	int fd;
	int page;
	PFbpage *bpage;
	struct PFhashentry *next;
} PFhashentry;

static PFhashentry *PFhashtbl[PF_HASH_SIZE];

static int PFhash(int fd, int page)
{
	return ((fd + page) % PF_HASH_SIZE);
}

void PFhashInit()
{
	int i;
	for (i = 0; i < PF_HASH_SIZE; i++)
		PFhashtbl[i] = NULL;
}

PFbpage *PFhashFind(int fd, int page)
{
	int bucket;
	PFhashentry *entry;

	bucket = PFhash(fd, page);
	for (entry = PFhashtbl[bucket]; entry != NULL; entry = entry->next) {
		if (entry->fd == fd && entry->page == page)
			return (entry->bpage);
	}
	return NULL;
}

int PFhashInsert(int fd, int page, PFbpage *bpage)
{
	int bucket;
	PFhashentry *entry;

	if (PFhashFind(fd, page) != NULL) {
		PFerrno = PFE_HASHPAGEEXIST;
		return PFE_HASHPAGEEXIST;
	}

	bucket = PFhash(fd, page);
	entry = (PFhashentry *)malloc(sizeof(PFhashentry));
	if (entry == NULL) {
		PFerrno = PFE_NOMEM;
		return PFE_NOMEM;
	}

	entry->fd = fd;
	entry->page = page;
	entry->bpage = bpage;
	entry->next = PFhashtbl[bucket];
	PFhashtbl[bucket] = entry;

	return PFE_OK;
}

int PFhashDelete(int fd, int page)
{
	int bucket;
	PFhashentry *entry, *prev;

	bucket = PFhash(fd, page);
	prev = NULL;
	for (entry = PFhashtbl[bucket]; entry != NULL; prev = entry, entry = entry->next) {
		if (entry->fd == fd && entry->page == page) {
			if (prev == NULL)
				PFhashtbl[bucket] = entry->next;
			else
				prev->next = entry->next;
			free(entry);
			return PFE_OK;
		}
	}

	PFerrno = PFE_HASHNOTFOUND;
	return PFE_HASHNOTFOUND;
}

void PFhashPrint()
{
	int i;
	PFhashentry *entry;

	printf("Hash Table Contents:\n");
	for (i = 0; i < PF_HASH_SIZE; i++) {
		printf("Bucket %d:\n", i);
		for (entry = PFhashtbl[i]; entry != NULL; entry = entry->next) {
			printf("  fd: %d, page: %d\n", entry->fd, entry->page);
		}
	}
}
