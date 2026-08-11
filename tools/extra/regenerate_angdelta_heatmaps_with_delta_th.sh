#!/bin/bash
#
# Usage: bash regenerate_angdelta_heatmaps_with_delta_th.sh [-h|--help]
#            [--strength-th F] [--count-th F] [--delta-th R]
#
# --strength-th F  passed to gen_angdelta_data.py's --strength-th (default: 0.25)
# --count-th F     passed to gen_angdelta_data.py's --count-th    (default: 0.025)
# --delta-th R     passed to gen_angdelta_data.py's --delta-th    (default: 0.25)

set -e

STRENGTH_TH="0.25"
COUNT_TH="0.025"
DELTA_TH="0.25"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            sed -n '2,8p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        --strength-th)
            STRENGTH_TH="$2"
            shift 2
            ;;
        --count-th)
            COUNT_TH="$2"
            shift 2
            ;;
        --delta-th)
            DELTA_TH="$2"
            shift 2
            ;;
        *)
            echo "Unrecognised argument: $1" >&2
            echo "Run with -h/--help for usage." >&2
            exit 1
            ;;
    esac
done

# Build the same filename suffix that gen_angdelta_data.py's threshold_suffix()
# encodes into its output filenames, e.g. "st-0p250-ct-0p025-dt-0p250", so the
# subsequent visualize_heatmap.py calls pick up the right file.
fmt_th() {
    printf '%.3f' "$1" | tr '.' 'p'
}
SUFFIX="st-$(fmt_th "$STRENGTH_TH")-ct-$(fmt_th "$COUNT_TH")-dt-$(fmt_th "$DELTA_TH")"

# Short human-readable label for each condition, used to build each plot's title.
TITLE_SUFFIX="(st=$(printf '%.3f' "$STRENGTH_TH"), ct=$(printf '%.3f' "$COUNT_TH"), dt=$(printf '%.3f' "$DELTA_TH"))"
CONDS=(evolve-20X-400gen-400pop-100epi-2000its evolve-10B-400gen-400pop-100epi-2000its evolve-20X-10B-400gen-400pop-100epi-2000its)
LABELS=("20 barriers" "10 bridges" "20 barriers & 10 bridges")

for i in "${!CONDS[@]}"; do
    COND="${CONDS[$i]}"
    LABEL="${LABELS[$i]}"
    BASENAME="size-10-thresh-${COND}-vs-baseline"

    ~/polybee/tools/gen_angdelta_data.py \
        "${COND}/bee-flowmaps-agg/bee-flowmap-size-10-intra-condition-merged-${COND}.csv" \
        baseline-runs-2000its/bee-flowmaps-agg/bee-flowmap-size-10-intra-condition-merged-baseline-runs-2000its.csv \
        --strength-th "$STRENGTH_TH" --count-th "$COUNT_TH" --delta-th "$DELTA_TH" --x-below-th \
        --basename "$BASENAME"

    ~/polybee/tools/visualize_heatmap.py \
        --config baseline-runs-2000its/baseline-runs-2000its.cfg --color-scale-max 1.57 --x-below-th --save-only \
        --title "${LABEL} vs baseline ${TITLE_SUFFIX}" \
        "${BASENAME}-angdelta-heatmap-${SUFFIX}.csv"
done
