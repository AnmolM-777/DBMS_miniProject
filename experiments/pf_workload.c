#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../pflayer/pf.h"

#define PAGE_COUNT 64
#define OPERATION_COUNT 2000

static int run_workload(const char *path, int strategy, int read_percent)
{
	int fd, page, i, error;
	char *buffer;
	PFStats stats;

	PF_InitWithConfig(8, strategy);
	PF_DestroyFile((char *)path);
	error = PF_CreateFile((char *)path);
	if (error != PFE_OK)
		return error;
	fd = PF_OpenFileWithConfig((char *)path, 8, strategy);
	if (fd < 0)
		return fd;

	for (i = 0; i < PAGE_COUNT; i++) {
		error = PF_AllocPage(fd, &page, &buffer);
		if (error != PFE_OK)
			return error;
		buffer[0] = (char)i;
		error = PF_UnfixPage(fd, page, TRUE);
		if (error != PFE_OK)
			return error;
	}

	error = PF_CloseFile(fd);
	if (error != PFE_OK)
		return error;
	fd = PF_OpenFileWithConfig((char *)path, 8, strategy);
	if (fd < 0)
		return fd;
	PF_ResetStats();
	for (i = 0; i < OPERATION_COUNT; i++) {
		page = (i * 17) % PAGE_COUNT;
		error = PF_GetThisPage(fd, page, &buffer);
		if (error != PFE_OK)
			return error;
		if ((i % 100) < read_percent) {
			error = PF_UnfixPage(fd, page, FALSE);
		} else {
			buffer[0] = (char)(buffer[0] + 1);
			error = PF_UnfixPage(fd, page, TRUE);
		}
		if (error != PFE_OK)
			return error;
	}

	PF_GetStats(&stats);
	printf("%s,%s,%d,%ld,%ld,%ld,%ld,%ld\n",
		strategy == PF_REPLACEMENT_LRU ? "LRU" : "MRU",
		path, read_percent, stats.logical_reads, stats.logical_writes,
		stats.physical_reads, stats.physical_writes, stats.pages_accessed);
	error = PF_CloseFile(fd);
	if (error == PFE_OK)
		PF_DestroyFile((char *)path);
	return error;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "pf_workload.db";
	int read_percent;
	int strategy;

	printf("strategy,path,read_percent,logical_reads,logical_writes,"
	       "physical_reads,physical_writes,pages_accessed\n");
	for (strategy = PF_REPLACEMENT_LRU; strategy <= PF_REPLACEMENT_MRU;
	     strategy++) {
		for (read_percent = 0; read_percent <= 100; read_percent += 20) {
			if (run_workload(path, strategy, read_percent) != PFE_OK) {
				PF_PrintError("PF workload");
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}
