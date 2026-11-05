#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "am.h"
#include "pf.h"

/*
 * Build an integer index from a student file whose first column is the
 * record id and whose second column is the roll number. The input must be
 * sorted by roll number; callers can sort a copy before invoking this
 * function so sorting time is included in their benchmark.
 */
int AM_BulkLoadSortedStudent(const char *studentFile, const char *indexName)
{
	FILE *input;
	char line[4096];
	char *field;
	char *end;
	long rec_id;
	long roll_no;
	long previous_roll = 0;
	int first = TRUE;
	int fd;
	int error;
	char index_path[512];

	if (studentFile == NULL || indexName == NULL)
		return AME_INVALIDVALUE;
	input = fopen(studentFile, "r");
	if (input == NULL)
		return AME_PF;
	error = AM_CreateIndex((char *)indexName, 0, 'i', sizeof(int));
	if (error != AME_OK) {
		fclose(input);
		return error;
	}
	snprintf(index_path, sizeof(index_path), "%s.0", indexName);
	fd = PF_OpenFile(index_path);
	if (fd < 0) {
		fclose(input);
		return AME_PF;
	}

	if (fgets(line, sizeof(line), input) == NULL) {
		fclose(input);
		PF_CloseFile(fd);
		return AME_INVALIDVALUE;
	}
	while (fgets(line, sizeof(line), input) != NULL) {
		field = strtok(line, ";");
		if (field == NULL)
			continue;
		rec_id = strtol(field, &end, 10);
		if (*end != '\0' && *end != '\r' && *end != '\n')
			continue;
		field = strtok(NULL, ";");
		if (field == NULL)
			continue;
		roll_no = strtol(field, &end, 10);
		if (*end != '\0' && *end != '\r' && *end != '\n')
			continue;
		if (!first && roll_no < previous_roll) {
			PF_CloseFile(fd);
			fclose(input);
			return AME_INVALIDVALUE;
		}
		previous_roll = roll_no;
		first = FALSE;
		{
			int key = (int)roll_no;
			error = AM_InsertEntry(fd, 'i', sizeof(int), (char *)&key,
					       (int)rec_id);
		}
		if (error != AME_OK) {
			PF_CloseFile(fd);
			fclose(input);
			return error;
		}
	}
	fclose(input);
	return PF_CloseFile(fd) == PFE_OK ? AME_OK : AME_PF;
}
