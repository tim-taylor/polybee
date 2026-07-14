#!/usr/bin/env python3
"""Generate a barrier "flowmap" CSV from one or more polybee config files.

What it does:
    Scans one or more polybee .cfg files for `barrier` entries (expanding any
    repeats specified via the nrx,dx / nry,dy fields into individual barrier
    instances, exactly as Environment::initialiseBarriers does) and records,
    for each barrier instance, its midpoint and orientation (the angle of the
    line from (x1,y1) to (x2,y2), in radians). All barrier instances from all
    input configs are pooled and binned into a grid of `--cell-size` x
    `--cell-size` cells covering the environment dimensions (taken from the
    first config's env-width/env-height; other configs with differing
    dimensions are still processed but generate a warning).

    For each cell, the dominant orientation axis and the strength of
    alignment to that axis are then calculated using the same double-angle
    method as Flowmap::calculateFlow (barriers are headless lines, just like
    the movement axis in a bee-movement flowmap, so orientations are folded
    into the range [0, pi) before averaging). This produces a barrier
    "flowmap" directly comparable to the bee-movement flowmaps produced by
    Flowmap::print, showing where barriers are concentrated and what
    orientation(s) dominate in each cell.

    The result is written out in exactly the same CSV format as
    Flowmap::print: one row per cell-row (y), each cell encoded as
    "axis:strength:count" and comma-separated, so it can be visualised or
    merged (see merge_flowmaps.py) using the same tooling as ordinary
    bee-movement flowmaps.

Usage:
    ./gen_barrier_flowmap.py CONFIG [CONFIG ...] [--cell-size N] [--basename NAME]

    CONFIG           One or more polybee .cfg files to process (glob-expand
                     with your shell to pass many, e.g. runs/*/polybee.cfg).
    --cell-size N    Cell size in environment units (default: 25). Should
                     match flowmap-cell-size of whatever bee-movement
                     flowmap you are comparing against.
    --basename NAME  Basename for the output CSV file (default: output).
                     Writes NAME-barrier-flowmap.csv in the current
                     directory.

Example:
    ./gen_barrier_flowmap.py polybee.cfg --cell-size 25 --basename m3
    # -> writes m3-barrier-flowmap.csv
"""

import argparse
import math
import sys


def parse_barrier(value):
    """Parse a single `barrier` value (x1,y1:x2,y2[:nrx,dx[:nry,dy]]) and
    return a list of (midpoint_x, midpoint_y, orientation) tuples, one per
    repeat instance (orientation is unchanged by repeats, since repeats are
    just translations of the same barrier)."""
    parts = value.split(':')
    if len(parts) < 2:
        return []
    p1 = parts[0].split(',')
    p2 = parts[1].split(',')
    if len(p1) < 2 or len(p2) < 2:
        return []
    try:
        x1, y1 = float(p1[0]), float(p1[1])
        x2, y2 = float(p2[0]), float(p2[1])
    except ValueError:
        return []

    num_repeats_x, dx = 1, 0.0
    num_repeats_y, dy = 1, 0.0
    if len(parts) >= 3:
        p3 = parts[2].split(',')
        if len(p3) >= 2:
            try:
                num_repeats_x, dx = int(p3[0]), float(p3[1])
            except ValueError:
                num_repeats_x, dx = 1, 0.0
    if len(parts) >= 4:
        p4 = parts[3].split(',')
        if len(p4) >= 2:
            try:
                num_repeats_y, dy = int(p4[0]), float(p4[1])
            except ValueError:
                num_repeats_y, dy = 1, 0.0

    orientation = math.atan2(y2 - y1, x2 - x1)

    instances = []
    for i in range(num_repeats_x):
        tx1, tx2 = x1 + i * dx, x2 + i * dx
        for j in range(num_repeats_y):
            ty1, ty2 = y1 + j * dy, y2 + j * dy
            instances.append(((tx1 + tx2) / 2.0, (ty1 + ty2) / 2.0, orientation))
    return instances


def parse_config(filepath):
    """Return (env_width, env_height, barriers) where barriers is a list of
    (midpoint_x, midpoint_y, orientation) tuples."""
    env_width = None
    env_height = None
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
            elif key == 'barrier':
                barriers.extend(parse_barrier(value))

    return env_width, env_height, barriers


def build_flowmap(barriers, env_width, env_height, cell_size):
    """Bin barrier instances into a grid of cells. Returns (cells, nrows, ncols)
    where cells[row][col] is a dict with 'count' and 'thetas' (list of
    orientations recorded in that cell)."""
    ncols = math.ceil(env_width / cell_size)
    nrows = math.ceil(env_height / cell_size)
    cells = [[{'count': 0, 'thetas': []} for _ in range(ncols)] for _ in range(nrows)]

    for mid_x, mid_y, theta in barriers:
        col = int(mid_x) // cell_size
        row = int(mid_y) // cell_size
        col = min(max(col, 0), ncols - 1)
        row = min(max(row, 0), nrows - 1)
        cell = cells[row][col]
        cell['count'] += 1
        cell['thetas'].append(theta)

    return cells, nrows, ncols


def calculate_flow(cells, nrows, ncols):
    """Calculate the dominant orientation axis and alignment strength for each
    cell, using the same double-angle method as Flowmap::calculateFlow (see
    Flowmap.cpp). Returns a 2-D list of (axis, strength, count) tuples."""
    result = []
    for row in range(nrows):
        out_row = []
        for col in range(ncols):
            cell = cells[row][col]
            count = cell['count']
            if count > 0:
                sum_sin = sum(math.sin(2.0 * theta) for theta in cell['thetas'])
                sum_cos = sum(math.cos(2.0 * theta) for theta in cell['thetas'])
                avg_sin = sum_sin / count
                avg_cos = sum_cos / count
                axis = 0.5 * math.atan2(avg_sin, avg_cos)
                strength = math.sqrt(avg_sin ** 2 + avg_cos ** 2)
            else:
                axis, strength = 0.0, 0.0
            out_row.append((axis, strength, count))
        result.append(out_row)
    return result


def write_flowmap(filepath, flow, nrows, ncols):
    """Write the flowmap in the same format as Flowmap::print."""
    with open(filepath, 'w') as f:
        for row in range(nrows):
            line = ','.join(
                f"{axis:.5f}:{strength:.5f}:{count}"
                for axis, strength, count in flow[row][:ncols]
            )
            f.write(line + '\n')


def main():
    parser = argparse.ArgumentParser(
        description='Generate a barrier flowmap CSV (position, orientation, count per cell) '
                    'from polybee config files, in the same format as Flowmap::print.'
    )
    parser.add_argument('configs', nargs='+', help='Config file(s) to process')
    parser.add_argument(
        '--cell-size', type=int, default=25,
        metavar='N', help='Cell size in environment units (default: 25)'
    )
    parser.add_argument(
        '--basename', default='output',
        help='Basename for the output CSV file (default: output)'
    )
    args = parser.parse_args()

    if args.cell_size <= 0:
        print(f"Error: --cell-size must be positive, got {args.cell_size}", file=sys.stderr)
        sys.exit(1)

    all_barriers = []
    env_width = None
    env_height = None

    for cfg_path in args.configs:
        try:
            w, h, barriers = parse_config(cfg_path)
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

        all_barriers.extend(barriers)

    if env_width is None:
        print("Error: could not determine environment dimensions from any config file", file=sys.stderr)
        sys.exit(1)

    cells, nrows, ncols = build_flowmap(all_barriers, env_width, env_height, args.cell_size)
    flow = calculate_flow(cells, nrows, ncols)

    out_path = f"{args.basename}-barrier-flowmap.csv"
    write_flowmap(out_path, flow, nrows, ncols)
    print(f"Wrote {out_path}  ({len(all_barriers)} barriers, grid {ncols}x{nrows})")


if __name__ == '__main__':
    main()
