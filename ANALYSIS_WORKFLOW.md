# How to perform analysis and produce graphs of polybee experiments

## Analysis of a single condition across multiple runs

The whole pipeline below is automated by `tools/run_analysis.sh`. Run it from
a directory containing a `raw-output` subdirectory populated with the
`out-<basename>-*.txt` files produced by a series of evolutionary runs (see
`gen_slurm_file.py`). The script exits with an error if that input is
missing.

```
~/polybee/tools/run_analysis.sh \
    --basename evolve-20X-10B-400gen-400pop-100epi-2000its \
    --num-reps 50 \
    --title "Evolve positions of 20 barriers and 10 bridges" \
    --fitness-worst -0.50 \
    --fitness-best -0.78 \
    --flowmap-count-th 0.1 \
    --flowmap-strength-th 0.5 \
    --heatmap-cell-size 25
```

Run `run_analysis.sh --help` for the full list of options (raw-output
directory location, tools directory, target heatmap file, etc).

### Output layout

All generated output is organised into subdirectories alongside
`raw-output/`, created on demand:

| Directory                          | Contents                                                        |
|-------------------------------------|------------------------------------------------------------------|
| `fitness-data-indiv/`               | Per-run fitness CSVs (`ISLAND,GEN,FITNESS`)                      |
| `fitness-data-agg/`                 | Champion-fitness CSV, aggregated across runs                    |
| `fitness-graphs-indiv/`             | Per-run fitness graphs                                          |
| `fitness-graphs-agg/`               | Aggregate fitness graph (one-island summary across runs)         |
| `best-configs-indiv/`               | Best-individual `.cfg` file for each run                        |
| `bee-flowmaps-10-indiv/`, `-25-indiv/`, `-50-indiv/` | Raw per-run bee-movement flowmap output, one dir per cell size |
| `bee-flowmaps-agg/`                 | Merged/visualised bee-movement flowmaps                         |
| `barrier-and-bridge-maps-agg/`      | Barrier/bridge position heatmaps and barrier flowmaps            |

### What each stage does

1. **Fitness CSVs** - extracts `ISLAND,GEN,FITNESS` rows from each run's log
   into `fitness-data-indiv/`, and the champion fitness per run into
   `fitness-data-agg/`.
2. **Fitness graphs** - per-run graphs into `fitness-graphs-indiv/`;
   an aggregate one-island summary across runs into `fitness-graphs-agg/`.
3. **Best-individual configs** - one `.cfg` per run into
   `best-configs-indiv/`.
4. **Barrier/bridge heatmaps and barrier flowmaps** - built from the
   `best-configs-indiv/` configs, written to
   `barrier-and-bridge-maps-agg/` (for cell sizes 10, 25 and 50).
5. **Bee-movement flowmaps** - for each cell size, runs each replicate's
   best config through `polybee` into `bee-flowmaps-<N>-indiv/`, then
   merges across replicates into `bee-flowmaps-agg/`.

## Cross-analysis between two conditions

`tools/run-cross-analysis.sh` compares the merged bee-movement flowmaps of
two `run_analysis.sh` output directories (two experimental conditions), using
axial angular-delta heatmaps and histograms. Run it from the parent of the
two condition directories:

```
~/polybee/tools/run-cross-analysis.sh \
    --title "Barriers-only vs barriers-and-bridges" \
    condition-a-dir condition-b-dir
```

For each flowmap cell size found in both directories (detected from whatever
`bee-flowmap-size-*-intra-condition-merged-*.csv` files exist in each
directory's `bee-flowmaps-agg/`), it runs `gen_angdelta_data.py` six times:
with no thresholds and with the (configurable) default strength/count
thresholds `0.5`/`0.1`, each at histogram bin sizes of 5, 10 and 15 degrees.
Each resulting angdelta heatmap is visualised with `visualize_heatmap.py`
(fixed colour scale `0`-`pi/2`, using a config file taken from the first
condition directory's `best-configs-indiv/`, on the assumption that both
conditions share the same basic environment) and each histogram with
`visualize_angdelta_histogram.py`. The given `--title` is passed to every one
of these plots, with `(thresholds: count=..., strength=...)` appended for the
thresholded comparisons.

Output (18 heatmap CSV/PNG pairs and 18 histogram CSV/PNG pairs) is written
to `cross-analysis-<dir1>-vs-<dir2>/` by default, overridable with
`--output-dir`. Run `run-cross-analysis.sh --help` for the full list of
options.

# NOTES

Testing this stuff with data in:

```
/home/tim/tmp/polybee-data/evolve-20X-10B-400gen-400pop-100epi-2000its
```
