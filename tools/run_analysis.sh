#!/bin/bash
#
# Fully automates analysis of a set of PolyBee evolutionary-experiment runs:
# fitness CSVs/graphs, best-individual configs, barrier/bridge position
# heatmaps and flowmaps, and bee-movement flowmaps (per-run and merged
# across replicates).
#
# Must be run from a directory containing a raw-output subdirectory (or
# whatever --raw-output-dir points to) populated with out-<basename>-*.txt
# files produced by the evolutionary runs (see gen_slurm_file.py). All
# generated output is organised into subdirectories alongside raw-output/,
# which are created on demand:
#
#   fitness-data-indiv/            per-run fitness CSVs
#   fitness-data-agg/              champion-fitness CSV (aggregate across runs)
#   fitness-graphs-indiv/          per-run fitness graphs
#   fitness-graphs-agg/            aggregate fitness graphs
#   best-configs-indiv/            best-individual .cfg file per run
#   bee-flowmaps-<N>-indiv/        raw per-run bee-movement flowmap output,
#                                  N in {10, 25, 50}
#   bee-flowmaps-agg/              merged/visualised bee-movement flowmaps
#   barrier-and-bridge-maps-agg/   barrier/bridge heatmaps and flowmaps
#
# Usage:
#   run_analysis.sh --basename NAME --num-reps N --title "TITLE" [options]
#
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: run_analysis.sh --basename NAME --num-reps N --title "TITLE" [options]

Required:
  --basename NAME            Experiment basename (matches out-NAME-*.txt files)
  --num-reps N                Number of replicate runs
  --title "TITLE"              Human-readable title used in plot titles

Options:
  --raw-output-dir DIR        Directory containing out-*.txt files (default: raw-output)
  --fitness-worst VAL          Y-axis worst-fitness bound for plots (default: -0.50)
  --fitness-best VAL           Y-axis best-fitness bound for plots (default: -0.78)
  --flowmap-count-th VAL       Flowmap count threshold (default: 0.1)
  --flowmap-strength-th VAL    Flowmap strength threshold (default: 0.5)
  --heatmap-cell-size N        Cell size for barrier/bridge heatmaps (default: 25)
  --target-heatmap FILE        Target heatmap CSV used for bee-movement runs
                               (default: ~/polybee/target-heatmaps/target-heatmap-13x16-4-rows.csv)
  --tools-dir DIR              Directory containing the polybee analysis tools
                               (default: ~/polybee/tools)
  --polybee-run FILE            polybee "run" wrapper script (default: ~/polybee/run)
  -h, --help                   Show this help
EOF
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
EXPT_BASENAME=""
NUM_REPS=""
RUN_TITLE=""
RAW_OUTPUT_DIR="raw-output"
FITNESS_PLOT_WORST="-0.50"
FITNESS_PLOT_BEST="-0.78"
FLOWMAP_COUNT_TH="0.1"
FLOWMAP_STRENGTH_TH="0.5"
HEATMAP_CELL_SIZE=25
TARGET_HEATMAP="$HOME/polybee/target-heatmaps/target-heatmap-13x16-4-rows.csv"
TOOLS_DIR="$HOME/polybee/tools"
POLYBEE_RUN="$HOME/polybee/run"

while [ $# -gt 0 ]; do
    case "$1" in
        --basename) EXPT_BASENAME="$2"; shift 2 ;;
        --num-reps) NUM_REPS="$2"; shift 2 ;;
        --title) RUN_TITLE="$2"; shift 2 ;;
        --raw-output-dir) RAW_OUTPUT_DIR="$2"; shift 2 ;;
        --fitness-worst) FITNESS_PLOT_WORST="$2"; shift 2 ;;
        --fitness-best) FITNESS_PLOT_BEST="$2"; shift 2 ;;
        --flowmap-count-th) FLOWMAP_COUNT_TH="$2"; shift 2 ;;
        --flowmap-strength-th) FLOWMAP_STRENGTH_TH="$2"; shift 2 ;;
        --heatmap-cell-size) HEATMAP_CELL_SIZE="$2"; shift 2 ;;
        --target-heatmap) TARGET_HEATMAP="$2"; shift 2 ;;
        --tools-dir) TOOLS_DIR="$2"; shift 2 ;;
        --polybee-run) POLYBEE_RUN="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Error: unknown argument: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [ -z "$EXPT_BASENAME" ] || [ -z "$NUM_REPS" ] || [ -z "$RUN_TITLE" ]; then
    echo "Error: --basename, --num-reps and --title are all required." >&2
    usage >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Verify raw input is present
# ---------------------------------------------------------------------------
if [ ! -d "$RAW_OUTPUT_DIR" ]; then
    echo "Error: raw-output directory '$RAW_OUTPUT_DIR' not found. Expected it to contain" >&2
    echo "out-${EXPT_BASENAME}-*.txt files from the evolutionary runs." >&2
    exit 1
fi

shopt -s nullglob
raw_files=("$RAW_OUTPUT_DIR"/out-"$EXPT_BASENAME"-*.txt)
shopt -u nullglob
if [ ${#raw_files[@]} -eq 0 ]; then
    echo "Error: no files matching '${RAW_OUTPUT_DIR}/out-${EXPT_BASENAME}-*.txt' found." >&2
    echo "Run the evolutionary experiments first (see gen_slurm_file.py)." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Output directories
# ---------------------------------------------------------------------------
FITNESS_DATA_INDIV_DIR="fitness-data-indiv"
FITNESS_DATA_AGG_DIR="fitness-data-agg"
FITNESS_GRAPHS_INDIV_DIR="fitness-graphs-indiv"
FITNESS_GRAPHS_AGG_DIR="fitness-graphs-agg"
BEST_CONFIGS_INDIV_DIR="best-configs-indiv"
BEE_FLOWMAPS_AGG_DIR="bee-flowmaps-agg"
BARRIER_BRIDGE_MAPS_AGG_DIR="barrier-and-bridge-maps-agg"
FLOWMAP_CELL_SIZES=(10 25 50)

ensure_dir() {
    if [ ! -d "$1" ]; then
        echo "Creating directory: $1"
        mkdir "$1"
    fi
}

ensure_dir "$FITNESS_DATA_INDIV_DIR"
ensure_dir "$FITNESS_DATA_AGG_DIR"
ensure_dir "$FITNESS_GRAPHS_INDIV_DIR"
ensure_dir "$FITNESS_GRAPHS_AGG_DIR"
ensure_dir "$BEST_CONFIGS_INDIV_DIR"
ensure_dir "$BEE_FLOWMAPS_AGG_DIR"
ensure_dir "$BARRIER_BRIDGE_MAPS_AGG_DIR"
for SZ in "${FLOWMAP_CELL_SIZES[@]}"; do
    ensure_dir "bee-flowmaps-${SZ}-indiv"
done

# ---------------------------------------------------------------------------
# Step 1: per-run fitness CSVs + champion-fitness aggregate CSV
# ---------------------------------------------------------------------------
echo "== Generating per-run fitness CSVs =="
for N in $(seq 1 "$NUM_REPS"); do
    gawk '/INFO: isl/ {print $3","$5","$11}' "${RAW_OUTPUT_DIR}"/out-"${EXPT_BASENAME}"-*_"${N}".txt \
        > "${FITNESS_DATA_INDIV_DIR}/fitness-${EXPT_BASENAME}_${N}.csv"
done

echo "== Extracting champion fitness values =="
"${TOOLS_DIR}/gen_champion_fitness_csv.py" --basename "${EXPT_BASENAME}" "${RAW_OUTPUT_DIR}"/out-"${EXPT_BASENAME}"-*.txt
mv "champion-fitnesses-${EXPT_BASENAME}.csv" "${FITNESS_DATA_AGG_DIR}/"

# ---------------------------------------------------------------------------
# Step 2: per-run and aggregate fitness graphs
# ---------------------------------------------------------------------------
echo "== Plotting per-run fitness graphs =="
for N in $(seq 1 "$NUM_REPS"); do
    "${TOOLS_DIR}/plot_fitness.py" --type 1 --one-island --ymin "$FITNESS_PLOT_BEST" --ymax "$FITNESS_PLOT_WORST" \
        --minimal --save-only --title "${RUN_TITLE} (Run ${N})" --basename "${EXPT_BASENAME}" \
        "${FITNESS_DATA_INDIV_DIR}/fitness-${EXPT_BASENAME}_${N}.csv"
    mv "graph-fitness-${EXPT_BASENAME}_${N}.png" "${FITNESS_GRAPHS_INDIV_DIR}/"
done

echo "== Plotting aggregate fitness graphs =="
"${TOOLS_DIR}/plot_fitness.py" --type 1 --one-island --ymin "$FITNESS_PLOT_BEST" --ymax "$FITNESS_PLOT_WORST" \
    --minimal --save-only --title "${RUN_TITLE}" --basename "${EXPT_BASENAME}" \
    "${FITNESS_DATA_INDIV_DIR}"/fitness-"${EXPT_BASENAME}"_*.csv
mv "graph-fitness-${EXPT_BASENAME}-summary.png" "${FITNESS_GRAPHS_AGG_DIR}/"

# ---------------------------------------------------------------------------
# Step 3: best-individual config files
# ---------------------------------------------------------------------------
echo "== Extracting best-individual configs =="
for N in $(seq 1 "$NUM_REPS"); do
    for LOGFILE in "${RAW_OUTPUT_DIR}"/out-"${EXPT_BASENAME}"-*_"${N}".txt; do
        STEM=$(basename "$LOGFILE" .txt)
        "${TOOLS_DIR}/best_individual_to_cfg.py" "$LOGFILE" -o "${BEST_CONFIGS_INDIV_DIR}/best-${STEM#out-}.cfg"
    done
done

shopt -s nullglob
first_cfg_matches=("${BEST_CONFIGS_INDIV_DIR}"/best-"${EXPT_BASENAME}"-*_1.cfg)
shopt -u nullglob
if [ ${#first_cfg_matches[@]} -eq 0 ]; then
    echo "Error: no best-individual config found for run 1 (expected ${BEST_CONFIGS_INDIV_DIR}/best-${EXPT_BASENAME}-*_1.cfg)" >&2
    exit 1
fi
FIRST_CFG="${first_cfg_matches[0]}"

# ---------------------------------------------------------------------------
# Step 4: barrier/bridge position heatmaps and barrier flowmaps
# ---------------------------------------------------------------------------
echo "== Generating barrier/bridge heatmaps =="
"${TOOLS_DIR}/gen_bx_heatmaps.py" --cell-size "$HEATMAP_CELL_SIZE" \
    --basename "${BARRIER_BRIDGE_MAPS_AGG_DIR}/heatmap-${EXPT_BASENAME}" \
    "${BEST_CONFIGS_INDIV_DIR}"/best-"${EXPT_BASENAME}"-*.cfg

"${TOOLS_DIR}/visualize_heatmap.py" --title "Barrier position heatmap: ${RUN_TITLE} (${NUM_REPS} runs)" \
    --config "$FIRST_CFG" --save-only "${BARRIER_BRIDGE_MAPS_AGG_DIR}/heatmap-${EXPT_BASENAME}_barriers.csv"

"${TOOLS_DIR}/visualize_heatmap.py" --title "Bridge position heatmap: ${RUN_TITLE} (${NUM_REPS} runs)" \
    --config "$FIRST_CFG" --save-only "${BARRIER_BRIDGE_MAPS_AGG_DIR}/heatmap-${EXPT_BASENAME}_bridges.csv"

echo "== Generating barrier flowmaps =="
for FLOWMAP_CELL_SIZE in "${FLOWMAP_CELL_SIZES[@]}"; do
    BARRIER_FLOWMAP_CSV="${BARRIER_BRIDGE_MAPS_AGG_DIR}/barrier-flowmap-size-${FLOWMAP_CELL_SIZE}-${EXPT_BASENAME}-barrier-flowmap.csv"

    "${TOOLS_DIR}/gen_barrier_flowmap.py" --cell-size "$FLOWMAP_CELL_SIZE" \
        --basename "${BARRIER_BRIDGE_MAPS_AGG_DIR}/barrier-flowmap-size-${FLOWMAP_CELL_SIZE}-${EXPT_BASENAME}" \
        "${BEST_CONFIGS_INDIV_DIR}"/best-"${EXPT_BASENAME}"-*.cfg

    # NOTE: fixed to reference the barrier-flowmap CSV that gen_barrier_flowmap.py
    # actually just wrote above - the original workflow passed a differently
    # named file here (flowmap-<basename>-barrier-flowmap.csv) that was never
    # generated by any step, which would have failed with a file-not-found error.
    "${TOOLS_DIR}/visualize_flowmap.py" --color -c "$FIRST_CFG" \
        -t "Summary of evolved barrier positions" --save-only "$BARRIER_FLOWMAP_CSV"

    "${TOOLS_DIR}/visualize_flowmap.py" --color --count-th "$FLOWMAP_COUNT_TH" --strength-th "$FLOWMAP_STRENGTH_TH" \
        -c "$FIRST_CFG" \
        -t "Summary of evolved barrier positions (thresholds: count=${FLOWMAP_COUNT_TH}, strength=${FLOWMAP_STRENGTH_TH})" \
        --save-only "$BARRIER_FLOWMAP_CSV"
done

# ---------------------------------------------------------------------------
# Step 5: bee-movement flowmaps (per-run and merged across replicates)
# ---------------------------------------------------------------------------
echo "== Generating bee-movement flowmaps =="
for FLOWMAP_CELL_SIZE in "${FLOWMAP_CELL_SIZES[@]}"; do
    RUN_DIR="bee-flowmaps-${FLOWMAP_CELL_SIZE}-indiv"

    (
        cd "$RUN_DIR"
        for N in $(seq 1 "$NUM_REPS"); do
            "$POLYBEE_RUN" -c ../"${BEST_CONFIGS_INDIV_DIR}"/best-"${EXPT_BASENAME}"-*_"${N}".cfg \
                --flowmap-cell-size "$FLOWMAP_CELL_SIZE" \
                --target-heatmap-filename "$TARGET_HEATMAP" \
                --visualise false --log-dir .
        done
    )

    MERGED_CSV="${BEE_FLOWMAPS_AGG_DIR}/bee-flowmap-size-${FLOWMAP_CELL_SIZE}-intra-condition-merged-${EXPT_BASENAME}.csv"

    "${TOOLS_DIR}/merge_flowmaps.py" "$MERGED_CSV" "${RUN_DIR}"/"${EXPT_BASENAME}"-*-flowmap*csv

    "${TOOLS_DIR}/visualize_flowmap.py" --color -c "$FIRST_CFG" -t "Bee flowmap: ${RUN_TITLE}" --save-only "$MERGED_CSV"

    "${TOOLS_DIR}/visualize_flowmap.py" --color --count-th "$FLOWMAP_COUNT_TH" --strength-th "$FLOWMAP_STRENGTH_TH" \
        -c "$FIRST_CFG" -t "Bee flowmap: ${RUN_TITLE} (thresholds: count=${FLOWMAP_COUNT_TH}, strength=${FLOWMAP_STRENGTH_TH})" \
        --save-only "$MERGED_CSV"
done

echo "Done."
