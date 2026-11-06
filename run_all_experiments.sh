#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
DATA="${1:-$ROOT/data/student.txt}"
make -C "$ROOT" all
(cd "$ROOT/pflayer" && ./pf_benchmark)
(cd "$ROOT/hflayer" && ./hf_student_benchmark "$DATA")
(cd "$ROOT/amlayer" && ./am_student_benchmark "$DATA")
cp "$ROOT/pflayer/pf_buffer_results.csv" "$ROOT/results/pf_buffer_results.csv"
cp "$ROOT/hflayer/hf_space_results.csv" "$ROOT/results/hf_space_results.csv"
cp "$ROOT/amlayer/am_index_results.csv" "$ROOT/results/am_index_results.csv"
python3 "$ROOT/scripts/generate_plots.py"
echo "All experiments completed. See $ROOT/results and layer CSV files."
