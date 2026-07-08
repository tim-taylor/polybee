#!/usr/bin/env python3
"""Generate an axial angular-delta heatmap from two bee-movement flowmaps.

What it does:
    Takes two flowmap CSV files, such as those produced by Flowmap::print
    (see Flowmap.cpp) or merge_flowmaps.py, and computes for each cell the
    axial angular delta between the two flowmaps' predominant movement axes:

        delta = 0.5 * arccos(cos(2 * (axis1 - axis2)))

    This treats the axes as headless (a direction and its opposite are
    equivalent), matching the axial nature of Flowmap's own axis values, and
    yields a delta in the range [0, pi/2] (0 = identical axis, pi/2 =
    perpendicular axes).

    A cell is scored 0 in the output if either flowmap's cell falls below
    the given thresholds:
      --strength-th  Minimum strength that at least one of the two flowmap
                      entries must reach for the cell to be scored. If both
                      entries have strength below this threshold, 0 is
                      recorded.
      --count-th     Minimum count that BOTH flowmap entries must reach for
                      the cell to be scored. If either entry has a count
                      below this threshold, 0 is recorded.

    The result is written as a plain (non-normalised) 2D CSV grid, one row
    per cell-row (y), matching the format used for heatmap CSVs elsewhere in
    this project (e.g. target-heatmaps/*.csv).

Usage:
    ./gen_angdelta-heatmap.py FLOWMAP1 FLOWMAP2 [--strength-th F] [--count-th N] [--basename NAME]

    FLOWMAP1, FLOWMAP2  Flowmap CSV files to compare (must have matching dimensions).
    --strength-th F     Minimum strength required of at least one flowmap entry
                        for a cell to be scored (float, 0.0-1.0). Default: 0.0
                        (no thresholding).
    --count-th N        Minimum count required of BOTH flowmap entries for a
                        cell to be scored (int). Default: 0 (no thresholding).
    --basename NAME     Basename for the output CSV file (default: output).
                        Writes NAME-angdelta-heatmap.csv in the current directory.

Example:
    ./gen_angdelta-heatmap.py run1-flowmap.csv run2-flowmap.csv --strength-th 0.3 --count-th 5 --basename cmp
    # -> writes cmp-angdelta-heatmap.csv
"""

import argparse
import math
import sys


def parse_flowmap(path):
    """Return a 2-D list (rows x cols) of (axis, strength, count) tuples."""
    cells = []
    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip('\n')
            if not line:
                continue
            row = []
            for token in line.split(','):
                parts = token.split(':')
                if len(parts) != 3:
                    sys.exit(f"Error: {path}:{lineno}: expected 'axis:strength:count', got {token!r}")
                try:
                    axis = float(parts[0])
                    strength = float(parts[1])
                    count = int(parts[2])
                except ValueError as e:
                    sys.exit(f"Error: {path}:{lineno}: {e}")
                row.append((axis, strength, count))
            cells.append(row)
    return cells


def angular_delta(axis1, axis2):
    """Axial angular delta between two headless axes, in [0, pi/2]."""
    return 0.5 * math.acos(math.cos(2.0 * (axis1 - axis2)))


def build_angdelta_heatmap(flowmap1, flowmap2, strength_th, count_th):
    """Return a 2-D list of angular-delta values (0.0 where thresholds aren't met)."""
    nrows = len(flowmap1)
    ncols = len(flowmap1[0])

    result = []
    for r in range(nrows):
        row = []
        for c in range(ncols):
            axis1, strength1, count1 = flowmap1[r][c]
            axis2, strength2, count2 = flowmap2[r][c]

            if strength1 < strength_th and strength2 < strength_th:
                row.append(0.0)
                continue
            if count1 < count_th or count2 < count_th:
                row.append(0.0)
                continue

            row.append(angular_delta(axis1, axis2))
        result.append(row)
    return result


def write_heatmap(cells, path):
    """Write a 2-D grid of floats as a plain CSV heatmap."""
    with open(path, 'w') as f:
        for row in cells:
            f.write(','.join(f"{value:.5f}" for value in row) + '\n')


def main():
    parser = argparse.ArgumentParser(
        description='Compute an axial angular-delta heatmap between two bee-movement flowmaps.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s run1-flowmap.csv run2-flowmap.csv
  %(prog)s run1-flowmap.csv run2-flowmap.csv --strength-th 0.3 --count-th 5 --basename cmp
        """
    )
    parser.add_argument('flowmap1', help='First flowmap CSV file')
    parser.add_argument('flowmap2', help='Second flowmap CSV file')
    parser.add_argument(
        '--strength-th', type=float, default=0.0, metavar='F',
        help='Minimum strength required of at least one flowmap entry for a cell '
             'to be scored (0.0-1.0). Cells where both entries fall below this are '
             'recorded as 0. Default: 0.0'
    )
    parser.add_argument(
        '--count-th', type=int, default=0, metavar='N',
        help='Minimum count required of BOTH flowmap entries for a cell to be scored. '
             'Cells where either entry falls below this are recorded as 0. Default: 0'
    )
    parser.add_argument(
        '--basename', default='output',
        help='Basename for the output CSV file (default: output)'
    )
    args = parser.parse_args()

    if not 0.0 <= args.strength_th <= 1.0:
        sys.exit(f"Error: --strength-th must be in range 0.0-1.0, got {args.strength_th}")
    if args.count_th < 0:
        sys.exit(f"Error: --count-th must be non-negative, got {args.count_th}")

    flowmap1 = parse_flowmap(args.flowmap1)
    flowmap2 = parse_flowmap(args.flowmap2)

    if not flowmap1 or not flowmap1[0]:
        sys.exit(f"Error: {args.flowmap1} is empty or contains no data")
    if not flowmap2 or not flowmap2[0]:
        sys.exit(f"Error: {args.flowmap2} is empty or contains no data")

    rows1, cols1 = len(flowmap1), len(flowmap1[0])
    rows2, cols2 = len(flowmap2), len(flowmap2[0])
    if (rows1, cols1) != (rows2, cols2):
        sys.exit(
            f"Error: dimension mismatch — {args.flowmap1} is {rows1}x{cols1} "
            f"but {args.flowmap2} is {rows2}x{cols2}"
        )

    heatmap = build_angdelta_heatmap(flowmap1, flowmap2, args.strength_th, args.count_th)

    out_path = f"{args.basename}-angdelta-heatmap.csv"
    write_heatmap(heatmap, out_path)
    print(f"Wrote {out_path}  (grid {cols1}x{rows1})")


if __name__ == '__main__':
    main()
