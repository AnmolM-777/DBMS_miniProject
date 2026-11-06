# DBMS Assignment 1 — ToyDB implementation

This repository extends the supplied ToyDB PF (Paged File) and AM (Access Method) layers and adds an HF (Heap File) layer for variable-length student records. It also includes reproducible experiments, CSV measurements, and generated plots.

## Requirements

- GCC and GNU Make
- Python 3
- Python packages: `matplotlib` (for plots)
- Linux, WSL, or another POSIX environment (the PF layer uses POSIX file APIs)

## Build

From the repository root:

```bash
make clean
make
```

The build produces the PF, AM, and HF test programs plus the three benchmark executables. Existing compiler warnings in some legacy AM routines may remain; the project builds successfully.

## Run all experiments

The supplied Student dataset is included at `data/student.txt`.

```bash
./run_all_experiments.sh
```

Or pass another copy of the assignment dataset:

```bash
./run_all_experiments.sh /path/to/student.txt
```

The script builds the project, runs all three benchmark programs, writes CSV files in their respective layer directories, copies the CSVs into `results/`, and generates the PNG figures in `results/`.

## PF layer — configurable buffer manager

### APIs

- `PF_SetBufferConfig(capacity, policy)` configures the buffer pool before opening files.
- `PF_OpenFileEx(filename, capacity, policy)` configures and opens a file in one call.
- `PF_REPLACEMENT_LRU` and `PF_REPLACEMENT_MRU` select the replacement policy.
- `PF_UnfixPage(fd, page, TRUE)` explicitly marks a page dirty while unfixing it.
- `PF_MarkDirty(fd, page)` marks a currently fixed page dirty; after calling it, unfix with `dirty=FALSE`.
- `PF_ResetStats`, `PF_GetStats`, and `PF_PrintStats` expose/reset the counters.

The buffer pool uses a hash table plus a doubly linked recency list. The head is most recently used and the tail is least recently used. LRU evicts from the tail; MRU evicts from the head. Fixed pages are never evicted. Dirty victims are written before reuse.

`PF_Stats` reports logical reads/writes, physical reads/writes, cache hits/misses, and evictions. Logical reads count successful page fetches; logical writes count dirty-page updates. Physical reads count page reads on buffer misses; physical writes count dirty-page writebacks on eviction or file close.

### PF benchmark

`pflayer/pf_benchmark.c` creates 128 pages, uses an 8-frame buffer pool, and performs 10,000 deterministic page operations at each write share (0%, 25%, 50%, 75%, 100%). It runs each mix with both LRU and MRU. The generated file is `pflayer/pf_buffer_results.csv`.

## HF layer — slotted pages and student records

The HF layer stores variable-length records in PF pages. Each page has a header and a slot directory; each slot stores a record offset and length. Insert reuses deleted slots where possible. Before inserting, the page is compacted so deleted-record holes are reclaimed. Delete invalidates the slot while preserving stable RIDs. `HF_OpenScan` / `HF_FindNextRec` provide sequential scans.

Run the student space experiment:

```bash
cd hflayer
./hf_student_benchmark ../data/student.txt
```

The benchmark inserts every non-empty student row (the data file's title line is skipped), scans the heap file back, and compares slotted-page utilization against fixed-size static records with maximum record lengths of 112, 128, 160, 192, and 256 bytes. The result is `hflayer/hf_space_results.csv`.

Utilization is measured as record payload plus slotted-page header/slot-directory overhead divided by allocated page bytes. Static utilization uses payload bytes divided by allocated page bytes under the fixed-size slot capacity. The supplied dataset contains 17,813 records.

## AM layer — Student roll-number B+ tree

The supplied `student.txt` file has no conventional header row. Its second semicolon-separated field is the 8-character roll-number field; the first field is a separate student identifier. The benchmark indexes the second field as a fixed-width character key (`CHAR_TYPE`, length 8), and uses the input row number as the integer record identifier. Duplicate roll numbers are retained as RID overflow chains.

Run:

```bash
cd amlayer
./am_student_benchmark ../data/student.txt
```

Three construction methods are measured:

1. **Existing-file pass:** create an index and populate it in one pass over the already populated Student file's records, preserving file order.
2. **Incremental insertion:** start with an empty index and insert the same records in a deterministic shuffled order.
3. **Bottom-up bulk loading:** sort the roll-number keys, group duplicates, pack leaf pages directly, and construct internal levels from child minimum keys. This is a true bottom-up page-packing bulk loader, not repeated calls to `AM_InsertEntry`.

For each method the benchmark measures build CPU time and PF page I/O. It then runs 100 deterministic equality lookups and reports query time, logical page accesses, physical reads/writes, and the number of correct point queries. The bulk-loaded index is also sequentially scanned and the benchmark verifies that all 17,813 record IDs are returned. Results are in `amlayer/am_index_results.csv`.

The AM search helpers use a straightforward ordered linear search within each page. Equality scans read the RID overflow chain directly while the page is fixed, avoiding a legacy page-buffer lifetime issue in the generic scan routine.

## Results and figures

CSV results are retained in each layer directory and copied into `results/`:

- `results/pf_buffer_results.csv`
- `results/hf_space_results.csv`
- `results/am_index_results.csv`

Figures:

- `results/pf_io_by_mix.png`
- `results/pf_hit_rate.png`
- `results/hf_space_utilization.png`
- `results/am_build_time.png`
- `results/am_query_page_accesses.png`

A written summary is in `docs/REPORT.md` and `docs/Assignment1_Report.pdf`.

## Notes

- The benchmark timings are from one run in the current environment; absolute times vary by machine and filesystem. Compare the trends and page counts, not just the exact seconds.
- `PF_OpenFileEx` configures the shared PF buffer pool. Configure it before opening the file(s) for the experiment; the pool is shared by open PF files.
- The heap-file format stores records as raw text lines for this assignment experiment. It does not parse the student fields into a C struct.
