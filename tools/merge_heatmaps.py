#!/usr/bin/env python3
"""
Merge multiple heatmap CSV files produced by Heatmap::printNormalised into a
single aggregate heatmap.

Each input file is a 2-D grid (one row per line, comma-separated), assumed to
already be normalised (see Heatmap.cpp / heatmap-normalised-*.csv output).
The merged heatmap's cell values are the mean, across all input files, of the
corresponding cell's value.

Usage:
    ./merge_heatmaps.py output.csv input1.csv input2.csv [input3.csv ...]
    ./merge_heatmaps.py merged.csv run-*/heatmap-normalised-*.csv
"""

import argparse
import sys


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


def merge(heatmaps):
    """
    Merge a list of heatmaps (each a 2-D list of floats).
    Returns a 2-D list of mean cell values.
    """
    num_rows = len(heatmaps[0])
    num_cols = len(heatmaps[0][0])
    n = len(heatmaps)

    result = []
    for r in range(num_rows):
        row = []
        for c in range(num_cols):
            total = sum(hm[r][c] for hm in heatmaps)
            row.append(total / n)
        result.append(row)
    return result


def write_heatmap(cells, path):
    """Write a merged heatmap in Heatmap::print(Normalised) format."""
    with open(path, 'w') as f:
        for row in cells:
            line = ','.join(f"{value:.8f}" for value in row)
            f.write(line + '\n')


def main():
    parser = argparse.ArgumentParser(
        description='Merge multiple heatmap CSV files into a single aggregate heatmap (mean per cell).',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s merged.csv run1/heatmap-normalised-*.csv run2/heatmap-normalised-*.csv
  %(prog)s output.csv a.csv b.csv c.csv
        """
    )
    parser.add_argument('output_file', help='Output CSV file')
    parser.add_argument('input_files', nargs='+', help='Input heatmap CSV files')
    args = parser.parse_args()

    if len(args.input_files) < 1:
        sys.exit("Error: at least one input file is required")

    print(f"Loading {len(args.input_files)} heatmap(s)...")
    heatmaps = []
    for path in args.input_files:
        hm = parse_heatmap(path)
        if not hm:
            sys.exit(f"Error: {path} is empty or contains no data")
        heatmaps.append((path, hm))

    # Validate dimensions
    ref_path, ref_hm = heatmaps[0]
    ref_rows = len(ref_hm)
    ref_cols = len(ref_hm[0])
    for path, hm in heatmaps[1:]:
        rows = len(hm)
        cols = len(hm[0]) if hm else 0
        if rows != ref_rows or cols != ref_cols:
            sys.exit(
                f"Error: dimension mismatch — {ref_path} is {ref_rows}x{ref_cols} "
                f"but {path} is {rows}x{cols}"
            )

    print(f"Grid size: {ref_rows} rows x {ref_cols} cols")

    merged = merge([hm for _, hm in heatmaps])
    write_heatmap(merged, args.output_file)
    print(f"Merged heatmap written to: {args.output_file}")


if __name__ == '__main__':
    main()
