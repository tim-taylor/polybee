#!/usr/bin/env python3
"""Generate a delta heatmap from two bee-position heatmaps.

What it does:
    Takes two heatmap CSV files, such as those produced by
    Heatmap::printNormalised (see Heatmap.cpp) or merge_heatmaps.py - each
    assumed to already be normalised - and computes, for each cell, the
    value of the first heatmap minus the value of the second:

        delta = heatmap1[cell] - heatmap2[cell]

    The two input heatmaps must have matching dimensions. The result is
    written as a plain 2D CSV grid, one row per cell-row (y), matching the
    format used for heatmap CSVs elsewhere in this project (e.g.
    target-heatmaps/*.csv). Cell values may be negative (where heatmap2 >
    heatmap1).

    The output filename is bee-heatmap-delta-{BASE1}-vs-{BASE2}.csv, where
    BASE1 and BASE2 are the basenames (filename, no directory or
    extension) of the two input files, written in the current directory.

Usage:
    ./gen_heatmap_delta.py HEATMAP1 HEATMAP2

    HEATMAP1, HEATMAP2  Normalised heatmap CSV files to compare (must have
                        matching dimensions).

Example:
    ./gen_heatmap_delta.py condition-a-heatmap.csv condition-b-heatmap.csv
    # -> writes bee-heatmap-delta-condition-a-heatmap-vs-condition-b-heatmap.csv
"""

import argparse
import sys
from pathlib import Path


def parse_heatmap(path):
    """Return a 2-D list (rows x cols) of float cell values."""
    cells = []
    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip('\n')
            if not line:
                continue
            row = []
            for token in line.split(','):
                try:
                    row.append(float(token))
                except ValueError as e:
                    sys.exit(f"Error: {path}:{lineno}: {e}")
            cells.append(row)
    return cells


def build_delta_heatmap(heatmap1, heatmap2):
    """Return a 2-D list of heatmap1[cell] - heatmap2[cell] values."""
    nrows = len(heatmap1)
    ncols = len(heatmap1[0])

    result = []
    for r in range(nrows):
        row = []
        for c in range(ncols):
            row.append(heatmap1[r][c] - heatmap2[r][c])
        result.append(row)
    return result


def write_heatmap(cells, path):
    """Write a 2-D grid of floats as a plain CSV heatmap."""
    with open(path, 'w') as f:
        for row in cells:
            f.write(','.join(f"{value:.8f}" for value in row) + '\n')


def main():
    parser = argparse.ArgumentParser(
        description='Compute a delta heatmap (heatmap1 - heatmap2) between two bee-position heatmaps.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s condition-a-heatmap.csv condition-b-heatmap.csv
        """
    )
    parser.add_argument('heatmap1', help='First heatmap CSV file')
    parser.add_argument('heatmap2', help='Second heatmap CSV file')
    args = parser.parse_args()

    heatmap1 = parse_heatmap(args.heatmap1)
    heatmap2 = parse_heatmap(args.heatmap2)

    if not heatmap1 or not heatmap1[0]:
        sys.exit(f"Error: {args.heatmap1} is empty or contains no data")
    if not heatmap2 or not heatmap2[0]:
        sys.exit(f"Error: {args.heatmap2} is empty or contains no data")

    rows1, cols1 = len(heatmap1), len(heatmap1[0])
    rows2, cols2 = len(heatmap2), len(heatmap2[0])
    if (rows1, cols1) != (rows2, cols2):
        sys.exit(
            f"Error: dimension mismatch — {args.heatmap1} is {rows1}x{cols1} "
            f"but {args.heatmap2} is {rows2}x{cols2}"
        )

    delta = build_delta_heatmap(heatmap1, heatmap2)

    base1 = Path(args.heatmap1).stem
    base2 = Path(args.heatmap2).stem
    output_path = f"bee-heatmap-delta-{base1}-vs-{base2}.csv"
    write_heatmap(delta, output_path)
    print(f"Wrote {output_path}  (grid {cols1}x{rows1})")


if __name__ == '__main__':
    main()
