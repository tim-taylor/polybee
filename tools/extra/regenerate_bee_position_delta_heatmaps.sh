#!/bin/bash
#
# Run visualize_heatmap.py on the pre-generated CSV file recording bee position deltas between an experimental condition and the baseline.
# Produces two heatmaps for each case, one with numbers displayed and the other without.
#
# For example, for the 10 bridges (10B) case, we are looking at the delta between:
#  ./evolve-10B-400gen-400pop-100epi-2000its/bee-heatmaps-agg/bee-heatmap-size-50-intra-condition-merged-evolve-10B-400gen-400pop-100epi-2000its.csv
# and
#  ./baseline-runs-2000its/bee-heatmaps-agg/bee-heatmap-size-50-intra-condition-merged-baseline-runs-2000its.csv
#
# Each heatmap is normalised, so sum of all cells in each heatmap is 1.0
#
# The colour scale multiplies these numbers by 100 to give a percentage
# For cell size 50, there are 16x13 cells on the heatmap, so the mean value for each cell if
# visitations are homogeneous is 0.00481, or 0.48077%

COLOR_SCALE_MAX="0.004"

for SN in "" "--show-numbers"; do

    ~/polybee/tools/visualize_heatmap.py -c baseline-runs-2000its/baseline-runs-2000its.cfg $SN --delta --color-scale-max $COLOR_SCALE_MAX --save-only --title "10 bridges vs baseline" cross-analysis-evolve-10B-400gen-400pop-100epi-2000its-vs-baseline-runs-2000its/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-10B-400gen-400pop-100epi-2000its-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs-2000its.csv

    ~/polybee/tools/visualize_heatmap.py -c baseline-runs-2000its/baseline-runs-2000its.cfg $SN --delta --color-scale-max $COLOR_SCALE_MAX --save-only --title "20 barriers vs baseline" cross-analysis-evolve-20X-400gen-400pop-100epi-2000its-vs-baseline-runs-2000its/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-20X-400gen-400pop-100epi-2000its-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs-2000its.csv

    ~/polybee/tools/visualize_heatmap.py -c baseline-runs-2000its/baseline-runs-2000its.cfg $SN --delta --color-scale-max $COLOR_SCALE_MAX --save-only --title "20 barriers vs baseline" cross-analysis-evolve-20X-10B-400gen-400pop-100epi-2000its-vs-baseline-runs-2000its/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-20X-10B-400gen-400pop-100epi-2000its-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs-2000its.csv

done
