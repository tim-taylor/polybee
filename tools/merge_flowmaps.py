#!/usr/bin/env python3
"""
Merge multiple flowmap CSV files produced by Flowmap::print into a single
aggregate flowmap.

Each cell in a flowmap CSV is encoded as "axis:strength:count", where:
  axis     -- predominant movement axis (radians, in [-pi/2, pi/2))
  strength -- alignment strength [0, 1]
  count    -- number of bee movements recorded

Merging recovers the underlying sin/cos sums using the double-angle identity
used in Flowmap::calculateFlow(), combines them across all input files, then
recomputes axis and strength for the merged cell.  This is mathematically
equivalent to merging the raw theta data.

Usage:
    ./merge_flowmaps.py output.csv input1.csv input2.csv [input3.csv ...]
    ./merge_flowmaps.py merged.csv run-*/flowmap-*.csv
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


def merge(flowmaps):
    """
    Merge a list of flowmaps (each a 2-D list of (axis, strength, count)).
    Returns a 2-D list of (merged_axis, merged_strength, total_count).
    """
    num_rows = len(flowmaps[0])
    num_cols = len(flowmaps[0][0])

    result = []
    for r in range(num_rows):
        row = []
        for c in range(num_cols):
            total_count = 0
            total_sin = 0.0
            total_cos = 0.0
            for fm in flowmaps:
                axis, strength, count = fm[r][c]
                if count > 0:
                    two_theta = 2.0 * axis
                    total_sin += count * strength * math.sin(two_theta)
                    total_cos += count * strength * math.cos(two_theta)
                    total_count += count

            if total_count > 0:
                avg_sin = total_sin / total_count
                avg_cos = total_cos / total_count
                merged_axis = 0.5 * math.atan2(avg_sin, avg_cos)
                merged_strength = math.sqrt(avg_sin ** 2 + avg_cos ** 2)
            else:
                merged_axis = 0.0
                merged_strength = 0.0

            row.append((merged_axis, merged_strength, total_count))
        result.append(row)
    return result


def write_flowmap(cells, path):
    """Write a merged flowmap in Flowmap::print format."""
    with open(path, 'w') as f:
        for row in cells:
            line = ','.join(f"{axis:.5f}:{strength:.5f}:{count}" for axis, strength, count in row)
            f.write(line + '\n')


def main():
    parser = argparse.ArgumentParser(
        description='Merge multiple flowmap CSV files into a single aggregate flowmap.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s merged.csv run1/flowmap-*.csv run2/flowmap-*.csv
  %(prog)s output.csv a.csv b.csv c.csv
        """
    )
    parser.add_argument('output_file', help='Output CSV file')
    parser.add_argument('input_files', nargs='+', help='Input flowmap CSV files')
    args = parser.parse_args()

    if len(args.input_files) < 1:
        sys.exit("Error: at least one input file is required")

    print(f"Loading {len(args.input_files)} flowmap(s)...")
    flowmaps = []
    for path in args.input_files:
        fm = parse_flowmap(path)
        if not fm:
            sys.exit(f"Error: {path} is empty or contains no data")
        flowmaps.append((path, fm))

    # Validate dimensions
    ref_path, ref_fm = flowmaps[0]
    ref_rows = len(ref_fm)
    ref_cols = len(ref_fm[0])
    for path, fm in flowmaps[1:]:
        rows = len(fm)
        cols = len(fm[0]) if fm else 0
        if rows != ref_rows or cols != ref_cols:
            sys.exit(
                f"Error: dimension mismatch — {ref_path} is {ref_rows}x{ref_cols} "
                f"but {path} is {rows}x{cols}"
            )

    print(f"Grid size: {ref_rows} rows x {ref_cols} cols")

    merged = merge([fm for _, fm in flowmaps])
    write_flowmap(merged, args.output_file)
    print(f"Merged flowmap written to: {args.output_file}")


if __name__ == '__main__':
    main()
