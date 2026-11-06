#!/usr/bin/env python3
"""Generate assignment figures from the benchmark CSVs (no synthetic data)."""
import csv
from pathlib import Path
import matplotlib.pyplot as plt
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "results"
OUT.mkdir(exist_ok=True)

def rows(path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f))

# PF: I/O counts versus write share, one curve per replacement policy.
pf = rows(ROOT / "pflayer" / "pf_buffer_results.csv")
fig, ax = plt.subplots()
for policy in ("LRU", "MRU"):
    subset = [r for r in pf if r["policy"] == policy]
    x = [int(r["write_percent"]) for r in subset]
    ax.plot(x, [int(r["physical_reads"]) for r in subset], marker="o", label=f"{policy} physical reads")
    ax.plot(x, [int(r["physical_writes"]) for r in subset], marker="s", linestyle="--", label=f"{policy} physical writes")
ax.set_xlabel("Write operations in workload (%)")
ax.set_ylabel("Physical page I/O operations")
ax.set_title("PF buffer I/O by read/write workload mix")
ax.grid(True, alpha=.3); ax.legend(); fig.tight_layout()
fig.savefig(OUT / "pf_io_by_mix.png", dpi=180); plt.close(fig)

# PF: cache hit rate by write percentage.
fig, ax = plt.subplots()
for policy in ("LRU", "MRU"):
    subset = [r for r in pf if r["policy"] == policy]
    x = [int(r["write_percent"]) for r in subset]
    y = [100.0 * int(r["cache_hits"]) / max(1, int(r["operations"])) for r in subset]
    ax.plot(x, y, marker="o", label=policy)
ax.set_xlabel("Write operations in workload (%)"); ax.set_ylabel("Buffer hit rate (%)")
ax.set_title("LRU versus MRU buffer hit rate"); ax.grid(True, alpha=.3); ax.legend(); fig.tight_layout()
fig.savefig(OUT / "pf_hit_rate.png", dpi=180); plt.close(fig)

# HF: slotted page vs fixed-length records.
hf = rows(ROOT / "hflayer" / "hf_space_results.csv")
labels = ["Slotted"] + [f"Static {r['max_record_length']} B" for r in hf if r["management"] == "static"]
vals = [float(hf[0]["utilization_percent"])] + [float(r["utilization_percent"]) for r in hf if r["management"] == "static"]
fig, ax = plt.subplots(); bars=ax.bar(labels, vals)
ax.set_ylabel("Space utilization (%)"); ax.set_title("Slotted-page versus static record storage")
ax.set_ylim(0,100); ax.grid(axis="y",alpha=.3); ax.tick_params(axis="x",rotation=15)
for b,v in zip(bars,vals): ax.text(b.get_x()+b.get_width()/2,b.get_height()+1,f"{v:.1f}%",ha="center")
fig.tight_layout(); fig.savefig(OUT / "hf_space_utilization.png",dpi=180); plt.close(fig)

# AM: construction time and 100 point-query time.
am = rows(ROOT / "amlayer" / "am_index_results.csv")
build = [r for r in am if not r["experiment"].endswith("_lookup")]
lookup = [r for r in am if r["experiment"].endswith("_lookup")]
fig, ax = plt.subplots(); labels=[r["experiment"].replace("_", " ") for r in build]
vals=[float(r["seconds"]) for r in build]; bars=ax.bar(labels,vals)
ax.set_ylabel("CPU time (seconds)"); ax.set_title("B+ tree construction time")
ax.grid(axis="y",alpha=.3); ax.tick_params(axis="x",rotation=15)
for b,v in zip(bars,vals): ax.text(b.get_x()+b.get_width()/2,b.get_height(),f"{v:.4f}",ha="center",va="bottom")
fig.tight_layout();fig.savefig(OUT/"am_build_time.png",dpi=180);plt.close(fig)
fig, ax = plt.subplots(); labels=[r["experiment"].replace("_lookup", "").replace("_", " ") for r in lookup]
vals=[int(r["logical_page_accesses"]) for r in lookup]; bars=ax.bar(labels,vals)
ax.set_ylabel("Logical page accesses (100 point queries)");ax.set_title("B+ tree point-query page accesses")
ax.grid(axis="y",alpha=.3);ax.tick_params(axis="x",rotation=15)
for b,v in zip(bars,vals):ax.text(b.get_x()+b.get_width()/2,b.get_height(),str(v),ha="center",va="bottom")
fig.tight_layout();fig.savefig(OUT/"am_query_page_accesses.png",dpi=180);plt.close(fig)
print("Generated figures in", OUT)
