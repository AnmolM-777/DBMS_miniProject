/* pf.h: externs and error codes for Paged File Interface*/
#ifndef TRUE
#define TRUE 1		
#endif
#ifndef FALSE
#define FALSE 0
#endif

/************** Error Codes *********************************/
#define PFE_OK		0	/* OK */
#define PFE_NOMEM	-1	/* no memory */
#define PFE_NOBUF	-2	/* no buffer space */
#define PFE_PAGEFIXED 	-3	/* page already fixed in buffer */
#define PFE_PAGENOTINBUF -4	/* page to be unfixed is not in the buffer */
#define PFE_UNIX	-5	/* unix error */
#define PFE_INCOMPLETEREAD -6	/* incomplete read of page from file */
#define PFE_INCOMPLETEWRITE -7	/* incomplete write of page to file */
#define PFE_HDRREAD	-8	/* incomplete read of header from file */
#define PFE_HDRWRITE	-9	/* incomplte write of header to file */
#define PFE_INVALIDPAGE -10	/* invalid page number */
#define PFE_FILEOPEN	-11	/* file already open */
#define	PFE_FTABFULL	-12	/* file table is full */
#define PFE_FD		-13	/* invalid file descriptor */
#define PFE_EOF		-14	/* end of file */
#define PFE_PAGEFREE	-15	/* page already free */
#define PFE_PAGEUNFIXED	-16	/* page already unfixed */

/* Internal error: please report to the TA */
#define PFE_PAGEINBUF	-17	/* new page to be allocated already in buffer */
#define PFE_HASHNOTFOUND -18	/* hash table entry not found */
#define PFE_HASHPAGEEXIST -19	/* page already exist in hash table */


/* page size */
#define PF_PAGE_SIZE	4096
#define PF_MIN_BUFS	1
#define PF_MAX_BUFS	20
#define PF_MAX_CONFIG_BUFS	128

typedef enum {
	PF_REPLACEMENT_LRU = 0,
	PF_REPLACEMENT_MRU = 1
} PFReplacementStrategy;

typedef struct PFStats {
	long logical_reads;
	long logical_writes;
	long physical_reads;
	long physical_writes;
	long pages_accessed;
} PFStats;

/* externs from the PF layer */
extern int PFerrno;		/* error number of last error */
extern void PF_Init();
extern void PF_InitWithConfig();
extern int PF_SetBufferConfig();
extern int PF_OpenFileWithConfig();
extern int PF_OpenFileEx();
extern int PF_MarkDirty();
extern void PF_GetStats();
extern void PF_ResetStats();
extern void PF_PrintError();
extern int PF_CreateFile();
extern int PF_DestroyFile();
extern int PF_OpenFile();
extern int PF_CloseFile();
extern int PF_GetFirstPage();
extern int PF_GetNextPage();
extern int PF_GetThisPage();
extern int PF_AllocPage();
extern int PF_DisposePage();
extern int PF_UnfixPage();
