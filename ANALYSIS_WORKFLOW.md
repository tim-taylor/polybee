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
directory location, tools directory, etc).

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
| `bee-flowmaps-10-indiv/`, `-25-indiv/`, `-50-indiv/` | Raw per-run bee-movement flowmap and heatmap output, one dir per cell size |
| `bee-flowmaps-agg/`                 | Merged/visualised bee-movement flowmaps                         |
| `bee-heatmaps-agg/`                 | Merged bee-position heatmaps, one per cell size                 |
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
5. **Bee-movement flowmaps and heatmaps** - for each cell size, runs each
   replicate's best config through `polybee` into `bee-flowmaps-<N>-indiv/`,
   with both `--flowmap-cell-size` and `--heatmap-cell-size` set to that
   cell size (and `target-heatmap-filename` cleared, since the best-individual
   configs carry a target sized for the original evolve run's own
   heatmap-cell-size, which would otherwise no longer match and abort the
   run). Flowmaps are merged across replicates into `bee-flowmaps-agg/`;
   heatmaps are merged (mean per cell) into `bee-heatmaps-agg/`.

## Cross-analysis between two conditions

`tools/run_cross_analysis.sh` compares the merged bee-movement flowmaps and
merged bee-position heatmaps of two `run_analysis.sh` output directories (two
experimental conditions). Run it from the parent of the two condition
directories:

```
~/polybee/tools/run_cross_analysis.sh \
    --title "Barriers-only vs barriers-and-bridges" \
    condition-a-dir condition-b-dir
```

Each condition directory must have both a `bee-flowmaps-agg/` and a
`bee-heatmaps-agg/` subdirectory (as produced by `run_analysis.sh`), and a
`best-configs-indiv/` subdirectory. For each cell size found in both
directories (detected from whatever
`bee-flowmap-size-*-intra-condition-merged-*.csv` files exist in each
directory's `bee-flowmaps-agg/`):

- **Flowmap comparison** - `gen_angdelta_data.py` is run six times: with no
  thresholds and with the (configurable) default strength/count thresholds
  `0.5`/`0.1`, each at histogram bin sizes of 5, 10 and 15 degrees. Each
  resulting angdelta heatmap is visualised with `visualize_heatmap.py`
  (fixed colour scale `0`-`pi/2`, using a config file taken from the first
  condition directory's `best-configs-indiv/`, on the assumption that both
  conditions share the same basic environment) and each histogram with
  `visualize_angdelta_histogram.py`.
- **Heatmap comparison** - `gen_heatmap_delta.py` is run once (no
  thresholding - thresholds only make sense for the axial flowmap
  comparison above) on the two conditions' merged heatmaps from
  `bee-heatmaps-agg/`, producing a plain `heatmap1 - heatmap2` delta CSV.
  It's visualised with `visualize_heatmap.py --delta` (diverging
  blue-white-red colour scale fixed to `[-2.0, +2.0]`).

The given `--title` is passed to every plot, with
`(thresholds: count=..., strength=...)` appended for the thresholded
flowmap comparisons.

Output (18 angdelta heatmap CSV/PNG pairs and 18 histogram CSV/PNG pairs for
the flowmap comparison, plus one heatmap-delta CSV/PNG pair per cell size for
the heatmap comparison) is written to `cross-analysis-<dir1>-vs-<dir2>/` by
default, overridable with `--output-dir`. Run `run_cross_analysis.sh --help`
for the full list of options.

# NOTES

Testing this stuff with data in:

```
/home/tim/tmp/polybee-data/evolve-20X-10B-400gen-400pop-100epi-2000its
```

Example commands used to analysee data for tech report:

First the single-condition analyses:

```
~/polybee/tools/run_analysis.sh --basename evolve-10B-400gen-400pop-100epi-2000its --num-reps 50 --title "Evolve positions of 10 bridges"

~/polybee/tools/run_analysis.sh --basename evolve-20X-400gen-400pop-100epi-2000its --num-reps 50 --title "Evolve positions of 20 barriers"

~/polybee/tools/run_analysis.sh --basename evolve-20X-10B-400gen-400pop-100epi-2000its --num-reps 50 --title "Evolve positions of 20 barriers and 10 bridges"

~/polybee/tools/run_analysis.sh --basename "baseline-runs-2000its" --baseline --title "Baseline [no barriers or bridges] (5000 runs)"

```

And the cross-analyses:

```
run_cross_analysis.sh --title "Baseline vs Evolve positions for 10 bridges" [options] DIR1 DIR2
```