#!/usr/bin/env python3
"""Generate bridge and barrier heatmap CSVs from one or more polybee config files.

What it does:
    Scans one or more polybee .cfg files for `patch` and `barrier` entries and
    builds normalised 2D occupancy heatmaps of where "bridge" patches (patch
    type 10 with no repeat) and barriers are located within the environment.
    Each config's bridges/barriers are reduced to a single (x, y) centre point,
    all points from all configs are pooled, and each is binned into a grid of
    `--cell-size` x `--cell-size` cells covering the environment dimensions
    (taken from the first config's env-width/env-height; other configs with
    differing dimensions are still processed but generate a warning). Two CSV
    grids are written: one for bridge density, one for barrier density. Each
    cell value is the count of points in that cell divided by the total number
    of points of that kind, so each output grid sums to 1.0 (or is all zeros
    if no points of that kind were found).

    This is typically used to build a "ground truth" heatmap of bridge/barrier
    placement across many randomly generated config files, for comparison
    against simulated bee-visitation heatmaps (see visualize_heatmap's flowmap
    overlay).

Usage:
    ./gen_bx_heatmaps.py CONFIG [CONFIG ...] [--cell-size N] [--basename NAME]

    CONFIG           One or more polybee .cfg files to process (glob-expand
                     with your shell to pass many, e.g. runs/*/polybee.cfg).
    --cell-size N    Cell size in environment units (default: 30). Should
                     match the cell size used by whatever heatmap you are
                     comparing against.
    --basename NAME  Basename for output CSV files (default: heatmap).
                     Writes NAME_bridges.csv and NAME_barriers.csv in the
                     current directory.

Example:
    ./gen_bx_heatmaps.py polybee.cfg --cell-size 30 --basename m3
    # -> writes m3_bridges.csv and m3_barriers.csv
"""

import argparse
import csv
import math
import sys


def parse_config(filepath):
    env_width = None
    env_height = None
    bridges = []
    barriers = []

    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or '=' not in line:
                continue
            key, _, value = line.partition('=')
            key = key.strip()
            value = value.strip()

            if key == 'env-width':
                env_width = float(value)
            elif key == 'env-height':
                env_height = float(value)
            elif key == 'patch':
                # Format: x,y,w,h:type:p2:p3:repeat:p5:p6
                # Bridge: type == 10, repeat (parts[4]) <= 1
                parts = value.split(':')
                if len(parts) < 5:
                    continue
                if parts[1].strip() != '10':
                    continue
                try:
                    repeat = float(parts[4].strip())
                except ValueError:
                    continue
                if repeat > 1:
                    continue
                geom = parts[0].split(',')
                if len(geom) < 4:
                    continue
                try:
                    x, y, w, h = float(geom[0]), float(geom[1]), float(geom[2]), float(geom[3])
                except ValueError:
                    continue
                bridges.append((x + w / 2.0, y + h / 2.0))
            elif key == 'barrier':
                # Format: x1,y1:x2,y2  (may have extra colon-fields, ignored)
                parts = value.split(':')
                if len(parts) < 2:
                    continue
                p1 = parts[0].split(',')
                p2 = parts[1].split(',')
                if len(p1) < 2 or len(p2) < 2:
                    continue
                try:
                    x1, y1 = float(p1[0]), float(p1[1])
                    x2, y2 = float(p2[0]), float(p2[1])
                except ValueError:
                    continue
                barriers.append(((x1 + x2) / 2.0, (y1 + y2) / 2.0))

    return env_width, env_height, bridges, barriers


def build_heatmap(points, env_width, env_height, cell_size):
    ncols = math.ceil(env_width / cell_size)
    nrows = math.ceil(env_height / cell_size)
    grid = [[0] * ncols for _ in range(nrows)]
    for px, py in points:
        col = int(px / cell_size)
        row = int(py / cell_size)
        if 0 <= col < ncols and 0 <= row < nrows:
            grid[row][col] += 1
    return grid, nrows, ncols


def write_heatmap_csv(filepath, grid, nrows, ncols, total):
    with open(filepath, 'w', newline='') as f:
        writer = csv.writer(f)
        for row in range(nrows):
            writer.writerow([
                f"{grid[row][col] / total:.6f}" if total > 0 else "0.000000"
                for col in range(ncols)
            ])


def main():
    parser = argparse.ArgumentParser(
        description='Generate bridge and barrier heatmap CSVs from polybee config files.'
    )
    parser.add_argument('configs', nargs='+', help='Config file(s) to process')
    parser.add_argument(
        '--cell-size', type=int, default=30,
        metavar='N', help='Cell size in environment units (default: 30)'
    )
    parser.add_argument(
        '--basename', default='heatmap',
        help='Basename for output CSV files (default: heatmap)'
    )
    args = parser.parse_args()

    all_bridges = []
    all_barriers = []
    env_width = None
    env_height = None

    for cfg_path in args.configs:
        try:
            w, h, bridges, barriers = parse_config(cfg_path)
        except OSError as e:
            print(f"Warning: could not read {cfg_path}: {e}", file=sys.stderr)
            continue

        if w is None or h is None:
            print(f"Warning: env-width/env-height not found in {cfg_path}", file=sys.stderr)
            continue

        if env_width is None:
            env_width, env_height = w, h
        elif (env_width, env_height) != (w, h):
            print(
                f"Warning: env dimensions in {cfg_path} ({w}x{h}) differ from "
                f"first file ({env_width}x{env_height})",
                file=sys.stderr
            )

        all_bridges.extend(bridges)
        all_barriers.extend(barriers)

    if env_width is None:
        print("Error: could not determine environment dimensions from any config file", file=sys.stderr)
        sys.exit(1)

    cell_size = args.cell_size

    bridge_grid, nrows, ncols = build_heatmap(all_bridges, env_width, env_height, cell_size)
    bridge_out = f"{args.basename}_bridges.csv"
    write_heatmap_csv(bridge_out, bridge_grid, nrows, ncols, len(all_bridges))
    print(f"Wrote {bridge_out}  ({len(all_bridges)} bridges, grid {ncols}x{nrows})")

    barrier_grid, nrows, ncols = build_heatmap(all_barriers, env_width, env_height, cell_size)
    barrier_out = f"{args.basename}_barriers.csv"
    write_heatmap_csv(barrier_out, barrier_grid, nrows, ncols, len(all_barriers))
    print(f"Wrote {barrier_out}  ({len(all_barriers)} barriers, grid {ncols}x{nrows})")


if __name__ == '__main__':
    main()
