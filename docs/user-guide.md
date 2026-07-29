# User Guide

This guide covers how to configure and run PolyBee, in both its normal
(single simulation) mode and its evolve (optimization) mode, and describes
the output files each mode produces.

- [Building and running](#building-and-running)
- [Configuring PolyBee](#configuring-polybee)
- [Running a normal simulation](#running-a-normal-simulation)
- [Running in evolve mode](#running-in-evolve-mode)
- [Output files](#output-files)

## Building and running

PolyBee is built with CMake and Ninja:

```
./make-debug     # configures and builds a debug binary at build/bin/debug/polybee
./run-debug      # runs it, passing through any command-line arguments
```

(`./make-release` / `./run-release` build and run an optimized `-O3` build
instead.) `run-debug`/`run-release` are thin wrappers that just invoke the
built `polybee` binary, so any command-line arguments described below can be
passed straight through, e.g. `./run-debug --num-bees 20`.

`polybee --help` lists every available parameter with its description and
default value.

## Configuring PolyBee

Parameters can be set in a config file, on the command line, or both:

- By default PolyBee reads `polybee.cfg` from the current directory. A
  different file can be specified with `-c`/`--config-filename`.
- Any parameter can also be given on the command line, e.g.
  `--num-bees 20 --evolve true`.
- **Command-line values take precedence over the config file** — if the same
  parameter is set in both places, the command-line value wins. This makes
  it easy to keep a base config file and override one or two values for a
  particular run, e.g. `./run-debug -c polybee.cfg --num-bees 100`.
- Parameters not set in either place fall back to their built-in defaults
  (shown by `polybee --help`).

A config file is a plain list of `key=value` lines; `#` starts a comment.

### Parameter groups

The parameters registered in `Params` (see `polybee --help` for the
authoritative, up-to-date list with defaults) fall into these groups:

| Group | Example parameters | Purpose |
|---|---|---|
| Simulation control | `num-iterations`, `rng-seed` | How long to run, and RNG seeding |
| Environment | `env-width`, `env-height` | Overall environment size |
| Tunnel | `tunnel-width`, `tunnel-height`, `tunnel-x`, `tunnel-y`, `tunnel-entrance` | Polytunnel geometry and entrances |
| Tunnel exit nets | `net-antibird-exit-prob`, `net-antihail-exit-prob`, `net-antibird-max-exit-attempts`, `net-antihail-max-exit-attempts` | Per-attempt exit probability and attempt limits for bees passing through netted entrances (see `PARAM-NOTES.md` for how the defaults were derived from the literature) |
| Barriers | `barrier`, `barrier-pass-prob` | Obstacles that block or partially block bee movement |
| Plant patches / flowers | `patch`, `plant-default-spacing`, `plant-default-jitter`, `flower-initial-nectar`, `min-visit-count-success`, `max-visit-count-success` | Where flowers are placed and what counts as a "successful" visit |
| Bees | `num-bees`, `bee-max-dir-delta`, `bee-step-length`, `bee-visual-range`, `bee-visit-memory-length`, `bee-prob-visit-nearest-flower`, `bee-in-hive-duration`, `bee-initial-energy`, `bee-energy-*` , `bee-on-flower-duration`, `bee-path-record-len` | Bee movement, sensing, and energy/foraging-bout behaviour |
| Hives | `hive` | Hive location(s) and exit direction |
| Evolve/optimization | `evolve`, `evolve-objective`, `evolve-spec`, `target-heatmap-filename`, `num-trials-per-config`, `num-configs-per-gen`, `num-generations`, `num-islands`, `migration-*`, `use-diverse-algorithms`, `bridge-overlaps-allowed` | See [Running in evolve mode](#running-in-evolve-mode) |
| Logging/output | `logging`, `log-dir`, `log-filename-prefix`, `heatmap-cell-size`, `flowmap-cell-size`, `flowmap-update-period` | Where and whether output files are written, and their resolution |
| Visualisation | `visualise`, `vis-cell-size`, `vis-delay-per-step`, `vis-bee-path-draw-len` | Real-time graphical display |

### Multi-value parameters

A few parameters describe *things placed in the environment* and can be
repeated on the command line or in the config file (one per occurrence) to
add multiple instances. Each uses a compact positional string format:

- **`hive=x,y:d`** — a hive at environment position `(x,y)`, with exit
  direction `d` (`0`=North, `1`=East, `2`=South, `3`=West, `4`=Random). At
  least one hive must be specified (unless hive positions are being evolved
  — see below). Example: `--hive 300,650:0 --hive 100,100:4`.

- **`tunnel-entrance=e1,e2:s[:t]`** — a tunnel entrance spanning positions
  `e1` to `e2` (measured along the specified side, from one end of that
  side) on side `s` (`0`=North, `1`=East, `2`=South, `3`=West). Optional `t`
  sets the net type across the entrance: `0`=none (default), `1`=anti-bird,
  `2`=anti-hail. Example: `--tunnel-entrance 5.5,10.0:0:1`.

- **`barrier=x1,y1:x2,y2[:nrx,dx[:nry,dy]]`** — a barrier line from
  `(x1,y1)` to `(x2,y2)` in environment coordinates. Optional `nrx,dx` /
  `nry,dy` repeat the barrier `nrx` times with spacing `dx` along the x
  axis, and/or `nry` times with spacing `dy` along the y axis, letting one
  `barrier=` entry describe a regular grid/row of barriers. Example:
  `--barrier 0,0:100,0:2,50:3,50`.

- **`patch=x,y,w,h:r[:j[:s[:n:dx,dy[:i]]]]`** — a rectangular patch of
  flowers with top-left corner `(x,y)`, width `w`, height `h`, and plant
  spacing `r`. Optional: `j` = jitter (std dev) between plant positions
  (default 0), `s` = species id (default 1), `n:dx,dy` = repeat the patch
  `n` times with offset `(dx,dy)` between repeats, `i` = `1` to exclude
  this patch's flowers from the successful-visit-fraction calculation
  (default `0`). Example: `--patch 200,200,50,400:2:0.5:1:3:100,0:1`.

See `polybee.cfg` and the files under `config-files/` for worked examples.

## Running a normal simulation

With `evolve=false` (the default), PolyBee runs a single simulation for
`num-iterations` steps using whatever hives, tunnel, barriers and patches
are configured, then (if `logging=true`) writes the [output
files](#output-files) described below.

If `visualise=true`, a Raylib window opens showing bees, their trails, and
the environment in real time. Controls in the visualisation window:

| Key | Action |
|---|---|
| `?` | Toggle the help overlay |
| `P` | Pause/unpause the simulation |
| `H` | Cycle display mode (bees, heatmap, or both) |
| `T` | Toggle bee trails on/off |
| `E` | Toggle EMD-to-target display on/off |
| `S` | Toggle successful-visit-fraction display on/off |
| `F` | Toggle flowmap overlay on/off |
| `1` / `2` | Colour bee trails randomly / by entrance used |
| `+` / `-` | Speed up / slow down the simulation |
| Mouse wheel | Zoom in/out |
| Arrow keys | Pan the camera |
| `R` | Reset camera zoom and position |
| `Esc` / close button | Exit |

## Running in evolve mode

Set `evolve=true` to run genetic optimization (via the
[Pagmo](https://esa.github.io/pagmo2/) library) instead of a single
simulation. Evolve mode repeatedly places elements of the environment
according to a candidate configuration, runs one or more simulation trials,
scores the result, and evolves the population of candidates over a number
of generations.

Key parameters:

- **`evolve-spec`** — what to evolve, in the format
  `[E:n,w][;][H:i,o,f][;][B:n,w][;][X:n,w]`:
  - `E:n,w` — evolve `n` tunnel entrance positions, each of width `w`.
  - `H:i,o,f` — evolve hive positions: `i` hives constrained inside the
    tunnel, `o` constrained outside it, `f` free to be placed either
    inside or outside.
  - `B:n,w` — evolve `n` "bridge" flower patches (of width `w`) intended to
    connect other patches.
  - `X:n,w` — evolve `n` barrier positions, each of width `w`.

  Any subset of these sections may be combined, separated by `;`, e.g.
  `evolve-spec=E:4,100.0;H:0,1,0` evolves 4 entrance positions (each 100
  units wide) and 1 hive position constrained to be outside the tunnel.
  Elements not covered by `evolve-spec` use their regular fixed
  configuration from `hive=`/`tunnel-entrance=`/`barrier=`/`patch=`.

- **`evolve-objective`** — what to optimize for:
  - `0` = minimize the earth mover's distance (EMD) between the run's bee
    position heatmap and a target heatmap (`target-heatmap-filename`,
    required in this mode — a CSV grid in the same format as the heatmap
    output files described below, sized to match `env-width`/`env-height`
    at `heatmap-cell-size`).
  - `1` = maximize the fraction of flowers receiving a "successful" number
    of visits (between `min-visit-count-success` and
    `max-visit-count-success`).

- **`num-configs-per-gen`**, **`num-trials-per-config`**,
  **`num-generations`** — population size per generation, number of
  simulation replicates run per candidate configuration (fitness is the
  median across replicates), and number of generations to evolve for.

- **`num-islands`**, **`migration-period`**, **`migration-num-select`**,
  **`migration-num-replace`**, **`use-diverse-algorithms`** — run several
  independent populations ("islands") in parallel, periodically migrating
  individuals between them. With `num-islands=1` there is a single
  population and no migration.

See `config-files/evolve-*.cfg` for complete worked examples, and
`ANALYSIS_WORKFLOW.md` plus [tools.md](tools.md) for how to run and analyse
many replicate evolve runs (e.g. on a Slurm cluster via
`tools/gen_slurm_file.py`).

## Output files

If `logging=true`, files are written to `log-dir` (default `.`), named
`<log-filename-prefix->` (if set) followed by a file-type tag and a
per-run timestamp string, e.g. `evolve-4-entrance-1o-hive-positions-flowmap-<timestamp>.csv`.

### Normal-mode output

Written once, at the end of the run:

| File | Format |
|---|---|
| `config-<ts>.cfg` | The full effective configuration for the run, in config-file format (can be reused directly as input via `-c`). |
| `heatmap-<ts>.csv` | Raw bee-position heatmap: a 2D grid (one row per line, comma-separated), each cell holding the count of bee positions recorded in that cell, at `heatmap-cell-size` resolution. |
| `heatmap-normalised-<ts>.csv` | The same grid, normalised so cell values sum to 1.0. |
| `flowmap-<ts>.csv` | Bee-movement flowmap: a 2D grid at `flowmap-cell-size` resolution, one row per line, cells comma-separated. Each cell is encoded `axis:strength:count`, where `axis` is the predominant movement axis through that cell in radians (headless, i.e. a direction and its opposite are treated as the same axis), `strength` is the alignment strength in `[0,1]`, and `count` is the number of bee movements recorded in the cell. Only written if the flowmap has data (`flowmap-update-period != 0`). |
| `run-info-<ts>.txt` | Human-readable run summary: PolyBee version and git commit, EMD to the target heatmap (if one was configured), successful-visit fraction, and tunnel-entrance crossing success rate. |

### Evolve-mode output

- `config-<ts>.cfg` — the base configuration, written once at the very start
  of the run (before any candidate-specific entrance/hive/bridge/barrier
  placement is applied).
- `evo-results-<ts>.txt` — written once, at the end of the run: for each
  island, the algorithm used, final population, champion decision vector
  and its fitness; and the overall best champion across all islands.
- **Per-generation progress is printed to stdout, not written to a file** —
  one line per evaluated configuration, in the form
  `isl <N> gen <N> evl <N> cnf <N> mdF <fitness> ...` (island number,
  generation, evaluation count, configuration number within the
  generation, median fitness across trials, followed by the evolved
  entrance/hive/bridge/barrier positions for that configuration). If you
  want this trace captured for later analysis (e.g. with
  `tools/run_analysis.sh`, which expects it in a `raw-output/out-<name>-*.txt`
  file), redirect it yourself, e.g. `./run-debug -c my.cfg | tee out-myrun.txt`
  — `tools/gen_slurm_file.py` sets this up automatically for Slurm batch
  runs via `#SBATCH --output=`.
- To turn a completed run's log into a directly runnable `.cfg` file for its
  best-found configuration, use `tools/best_individual_to_cfg.py` (see
  [tools.md](tools.md)).
