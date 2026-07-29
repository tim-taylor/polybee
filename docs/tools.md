# Tools

The `tools/` directory contains scripts for turning PolyBee's raw CSV/log
output into aggregated data and plots. Most are standalone Python scripts —
run `<script> --help` for the full option list; only a summary of purpose
and basic usage is given here. A few are documented in detail in
[`ANALYSIS_WORKFLOW.md`](https://github.com/tim-taylor/polybee/blob/main/ANALYSIS_WORKFLOW.md)
rather than repeated here.

- [Batch experiments and end-to-end analysis](#batch-experiments-and-end-to-end-analysis)
- [Extracting data from run logs](#extracting-data-from-run-logs)
- [Flowmap and heatmap generation](#flowmap-and-heatmap-generation)
- [Visualisation](#visualisation)
- [General-purpose plotting and stats](#general-purpose-plotting-and-stats)
- [Development utility](#development-utility)

## Batch experiments and end-to-end analysis

These are the scripts used to run and analyse a full set of replicate
evolve-mode experiments; see
**[`ANALYSIS_WORKFLOW.md`](https://github.com/tim-taylor/polybee/blob/main/ANALYSIS_WORKFLOW.md)**
for the full pipeline, output directory layout, and worked examples.

- **`gen_slurm_file.py`** — generates a Slurm batch script for running many
  replicate PolyBee evolve runs on a cluster, redirecting each replicate's
  stdout to `out-<name>-<jobid>_<n>.txt` (the raw log files the rest of the
  pipeline consumes).
  `./gen_slurm_file.py <experiment_name> [-n <replicates>] [-t <D-HH:MM:SS>]`

- **`run_analysis.sh`** — automates the full single-condition analysis
  pipeline (fitness CSVs and graphs, best-individual configs,
  barrier/bridge heatmaps, bee-movement flowmaps) from a directory of raw
  Slurm output logs. `run_analysis.sh --help` for options.

- **`run_cross_analysis.sh`** — compares the merged bee-movement flowmaps
  of two `run_analysis.sh` output directories (two experimental
  conditions) via angular-delta heatmaps and histograms.
  `run_cross_analysis.sh --help` for options.

## Extracting data from run logs

- **`gen_champion_fitness_csv.py`** — scans one or more `out-*_N.txt` Slurm
  log files for each run's final `Champion fitness: <value>` line and
  writes a `champion-fitnesses-<name>.csv` of `N,FITNESS` pairs, sorted
  best-first.
  `./gen_champion_fitness_csv.py out-myexpt-*_*.txt --basename myexpt`

- **`best_individual_to_cfg.py`** — converts an evolve-mode run log into a
  ready-to-run `.cfg` file for the best individual found in that run
  (scalar parameters from the log header, plus hive/entrance/barrier/bridge
  values taken from whichever were evolved).
  `./best_individual_to_cfg.py <logfile> [-o <output.cfg>] [-f <fitness>]`

## Flowmap and heatmap generation

- **`merge_flowmaps.py`** — merges multiple flowmap CSVs (e.g. per-replicate
  `flowmap-<ts>.csv` files) into one aggregate flowmap, correctly averaging
  the axial movement data rather than naively averaging axis/strength
  values.
  `./merge_flowmaps.py output.csv run-*/flowmap-*.csv`

- **`merge_heatmaps.py`** — merges multiple (normalised) heatmap CSVs into
  one aggregate heatmap, where each output cell is the mean of the
  corresponding input cells.
  `./merge_heatmaps.py output.csv run-*/heatmap-normalised-*.csv`

- **`gen_barrier_flowmap.py`** — builds a flowmap-format CSV showing where
  barriers are concentrated and their dominant orientation, from one or
  more `.cfg` files' `barrier=` entries, directly comparable to bee-movement
  flowmaps.
  `./gen_barrier_flowmap.py CONFIG [CONFIG ...] [--cell-size N] [--basename NAME]`

- **`gen_bx_heatmaps.py`** — builds normalised occupancy heatmaps of bridge
  and barrier placement across one or more `.cfg` files, for comparison
  against simulated bee-visitation heatmaps.
  `./gen_bx_heatmaps.py CONFIG [CONFIG ...] [--cell-size N] [--basename NAME]`

- **`gen_angdelta_data.py`** — compares two bee-movement flowmaps cell by
  cell, computing the axial angular delta between their predominant
  movement axes, and writes both a heatmap CSV and a binned histogram CSV
  of the (optionally strength/count-thresholded) deltas.
  `./gen_angdelta_data.py FLOWMAP1 FLOWMAP2 [--strength-th F] [--count-th F] [--bin-size D] [--basename NAME]`

- **`gen_heatmap_delta.py`** — compares two (normalised) bee-position
  heatmaps cell by cell, writing a heatmap CSV of `heatmap1 - heatmap2`
  (values may be negative). Output is named
  `bee-heatmap-delta-{BASE1}-vs-{BASE2}.csv` from the two input files'
  basenames.
  `./gen_heatmap_delta.py HEATMAP1 HEATMAP2`

## Visualisation

All of these require `matplotlib`/`numpy` (`pip install matplotlib numpy`,
or the Ubuntu/Debian `python3-matplotlib`/`python3-numpy` packages).

- **`visualize_heatmap.py`** — renders a heatmap CSV (bee-position heatmap,
  or any other 2D CSV grid such as an angular-delta heatmap) as an image,
  optionally overlaid with a flowmap and/or a config file's tunnel outline
  and hive locations. `--save-only` skips the on-screen display.
  `./visualize_heatmap.py <heatmap.csv> [-c polybee.cfg] [-f flowmap.csv] [--color-scale-max N] [--save-only]`

- **`visualize_flowmap.py`** — renders a flowmap CSV on its own (as a grid
  of oriented line segments) without an underlying heatmap.
  `./visualize_flowmap.py <flowmap.csv> [-c polybee.cfg] [--color] [--strength-th F] [--count-th F]`

- **`visualize_angdelta_histogram.py`** — draws a bar chart from a
  `gen_angdelta_data.py` histogram CSV.
  `./visualize_angdelta_histogram.py <histogram.csv> [--title TITLE] [--save-only]`

- **`combine_heatmaps.py`** — arranges several heatmap/flowmap PNGs (e.g.
  from `visualize_heatmap.py`) into a grid on a single A4 page, for
  side-by-side comparison. Also needs `pillow`.
  `./combine_heatmaps.py output.pdf image1.png image2.png ...`

## General-purpose plotting and stats

- **`plot_fitness.py`** — the tool used by `run_analysis.sh`: plots
  per-generation fitness (mean/median/min/individual) from one or more
  `island,generation,fitness` CSVs, with an interactive UI for toggling
  islands/metrics, or `--save-only` for batch use.
  `./plot_fitness.py [-t {0,1}] <fitness.csv> [--ymin N --ymax N] [--minimal] [--save-only --basename NAME]`

- **`plot_emd_scores.py`** / **`plot_emd_scores_islands.py`** — earlier,
  single-purpose versions of `plot_fitness.py` for EMD-score CSVs (single
  population / multi-island respectively). Not used by the automated
  pipeline, but usable standalone for one-off plots in the same CSV format.

- **`plot_boxplot.py`** — box-and-whisker plot of numeric values from one
  or more single-column CSV files, one box per file.
  `./plot_boxplot.py file1.csv [file2.csv ...] [--title T --labels A B]`

- **`plot_stats_chart.py`** — plots Q1/median/Q3/mean statistics against a
  `Reps` column from a CSV (needs `pandas` in addition to
  matplotlib/numpy).
  `./plot_stats_chart.py <input.csv> [--save output.png]`

- **`stats.py`** — prints mean, median and standard deviation of numbers
  read one-per-line from one or more files.

- **`sum-heatmap.awk`** — sums all values in a comma-separated heatmap CSV;
  useful as a sanity check that a normalised heatmap sums to ~1.0, e.g.
  `awk -f sum-heatmap.awk heatmap-normalised-<ts>.csv`.

## Development utility

- **`newclass.py`** — scaffolds a new `.h`/`.cpp` pair for a C++ class
  (unrelated to simulation output; a code-generation convenience for
  extending PolyBee itself). `./newclass.py ClassName`
