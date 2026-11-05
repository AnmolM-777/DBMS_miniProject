# toyDB Assignment 1

This project contains the Person A implementation for Assignment 1:

- configurable PF buffer pools with LRU and MRU replacement;
- dirty-page tracking, explicit `PF_MarkDirty`, and logical/physical I/O
  statistics;
- a reproducible read/write workload driver; and
- a sorted Student-file index loader for the AM layer.

The original PF API is preserved. Existing callers can continue to use
`PF_OpenFile` and `PF_UnfixPage`; experiments can use
`PF_OpenFileWithConfig`, `PF_InitWithConfig`, `PF_MarkDirty`, and the stats
functions declared in [`pflayer/pf.h`](pflayer/pf.h).

## Build

```sh
make
```

The legacy PF regression program can be built and run independently:

```sh
make -C pflayer testpf CC=gcc \
  CFLAGS="-std=gnu89 -Wall -Wextra -Wno-implicit-function-declaration"
cd pflayer && rm -f file1 file2 && ./testpf
```

## PF read/write experiment

The workload driver runs 0%, 20%, ..., 100% read mixes for both replacement
strategies with an eight-page pool and emits CSV:

```sh
./experiments/pf_workload results.db > pf-results.csv
```

The CSV columns are logical reads/writes, physical reads/writes, and pages
accessed. Plot `read_percent` against any of those columns for the assignment
graph. The temporary database is removed after each experiment.

## Sorted Student index

`amlayer/bulkload.c` provides `AM_BulkLoadSortedStudent`. It expects the
standard `student.txt` layout: a header followed by records with record ID in
column one and roll number in column two. The input must already be sorted by
roll number; sort a copy before timing so sorting time can be reported
separately. The function creates `<indexName>.0` and inserts each sorted key
with its source record ID.

`config.example` contains the runtime settings used by the experiments.
Do not commit generated `.o`, index, database, or result files.

## Validation

Run `make clean && make` before packaging. The legacy PF regression test and AM
smoke tests should be run from a clean checkout before submission.

## Review order

The changes are intentionally split into focused stacked pull requests: PF
API, replacement policy, statistics, workload experiments, AM bulk loading,
configuration, and documentation.
