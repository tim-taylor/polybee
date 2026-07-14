#!/usr/bin/env python3
"""Generate axial angular-delta heatmap and histogram data from two bee-movement flowmaps.

What it does:
    Takes two flowmap CSV files, such as those produced by Flowmap::print
    (see Flowmap.cpp) or merge_flowmaps.py, and computes for each cell the
    axial angular delta between the two flowmaps' predominant movement axes:

        delta = 0.5 * arccos(cos(2 * (axis1 - axis2)))

    This treats the axes as headless (a direction and its opposite are
    equivalent), matching the axial nature of Flowmap's own axis values, and
    yields a delta in the range [0, pi/2] (0 = identical axis, pi/2 =
    perpendicular axes).

    A cell is scored 0 in the output if either flowmap's cell is empty, or
    if either flowmap's cell falls below the given thresholds:
      (empty cell)   A cell with strength <= 0.0 or count == 0 in either
                      flowmap is always scored 0, regardless of the
                      thresholds below — matching the hard empty-cell
                      exclusion in visualize_flowmap.py / visualize_heatmap.py's
                      _draw_flowmap().
      --strength-th  Minimum strength that at least one of the two flowmap
                      entries must reach for the cell to be scored. If both
                      entries have strength below this threshold, 0 is
                      recorded.
      --count-th     Minimum count fraction that BOTH flowmap entries must
                      reach for the cell to be scored, where a cell's count
                      fraction is its count divided by the maximum count
                      found anywhere in its own flowmap (0.0-1.0), matching
                      the count-fraction thresholding in visualize_flowmap.py
                      / visualize_heatmap.py's _draw_flowmap(). If either
                      entry's count fraction falls below this threshold, 0
                      is recorded.

    The result is written as a plain (non-normalised) 2D CSV grid, one row
    per cell-row (y), matching the format used for heatmap CSVs elsewhere in
    this project (e.g. target-heatmaps/*.csv).

    A second file, NAME-angdelta-histogram-{suffix}.csv, is also written. It
    contains the data needed to plot a histogram of the weighted
    distribution of angular deltas across the heatmap. Deltas are binned
    into --bin-size degree-wide bins spanning [0, 90]. Cells that fail the
    thresholds above are excluded entirely (not just scored 0). For each bin
    the file records:

        bin_lo, bin_hi, weighted_count

    where weighted_count is the raw number of (thresholded) cells whose
    angular delta falls in the bin, multiplied by a normalised weighting for
    that bin. The weighting for a bin is the pooled occupancy of the cells
    in that bin divided by the pooled occupancy of all thresholded cells in
    the heatmap, where the pooled occupancy of a cell is the mean of its
    count in the two flowmaps.

    Both output filenames are tagged with the thresholds used to generate
    them: st-{strength_th} and ct-{count_th}, each to 3 decimal places with
    'p' in place of the decimal point, e.g.
    NAME-angdelta-heatmap-st-0p725-ct-0p400.csv.

Usage:
    ./gen_angdelta_data.py FLOWMAP1 FLOWMAP2 [--strength-th F] [--count-th F] [--bin-size D] [--basename NAME]

    FLOWMAP1, FLOWMAP2  Flowmap CSV files to compare (must have matching dimensions).
    --strength-th F     Minimum strength required of at least one flowmap entry
                        for a cell to be scored (float, 0.0-1.0). Default: 0.0
                        (no thresholding).
    --count-th F        Minimum count fraction required of BOTH flowmap entries
                        for a cell to be scored, where a cell's count fraction
                        is its count divided by the maximum count in its own
                        flowmap (float, 0.0-1.0). Default: 0.0 (no thresholding).
    --bin-size D        Histogram bin size in degrees, spanning [0, 90]
                        (float). Default: 15.
    --basename NAME     Basename for the output CSV files (default: output).
                        Writes NAME-angdelta-heatmap-{suffix}.csv and
                        NAME-angdelta-histogram-{suffix}.csv in the current
                        directory, where {suffix} encodes --strength-th and
                        --count-th (e.g. st-0p725-ct-0p400).

Example:
    ./gen_angdelta_data.py run1-flowmap.csv run2-flowmap.csv --strength-th 0.725 --count-th 0.4 --basename cmp
    # -> writes cmp-angdelta-heatmap-st-0p725-ct-0p400.csv and cmp-angdelta-histogram-st-0p725-ct-0p400.csv
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


def max_count(flowmap):
    """Maximum cell count found anywhere in a flowmap."""
    return max((count for row in flowmap for (_, _, count) in row), default=0)


def cell_is_valid(strength1, strength2, count1, count2, strength_th, count_th,
                  max_count1, max_count2):
    """Whether a cell meets the --strength-th / --count-th thresholds.

    count_th is a fraction (0.0-1.0) of each flowmap's own maximum count,
    matching the count-fraction thresholding in visualize_flowmap.py /
    visualize_heatmap.py's _draw_flowmap(), which also hard-excludes empty
    cells (strength <= 0.0 or count == 0) regardless of threshold — matched
    here too, so a cell with no observations in either flowmap is never
    scored as having an "identical axis" (delta 0) by default.
    """
    if strength1 <= 0.0 or count1 == 0 or strength2 <= 0.0 or count2 == 0:
        return False
    if strength1 < strength_th and strength2 < strength_th:
        return False
    fraction1 = count1 / max_count1 if max_count1 > 0 else 0.0
    fraction2 = count2 / max_count2 if max_count2 > 0 else 0.0
    if fraction1 < count_th or fraction2 < count_th:
        return False
    return True


def build_angdelta_heatmap(flowmap1, flowmap2, strength_th, count_th):
    """Return a 2-D list of angular-delta values (0.0 where thresholds aren't met)."""
    nrows = len(flowmap1)
    ncols = len(flowmap1[0])
    max_count1 = max_count(flowmap1)
    max_count2 = max_count(flowmap2)

    result = []
    for r in range(nrows):
        row = []
        for c in range(ncols):
            axis1, strength1, count1 = flowmap1[r][c]
            axis2, strength2, count2 = flowmap2[r][c]

            if not cell_is_valid(strength1, strength2, count1, count2, strength_th, count_th,
                                 max_count1, max_count2):
                row.append(0.0)
                continue

            row.append(angular_delta(axis1, axis2))
        result.append(row)
    return result


def build_angdelta_histogram(flowmap1, flowmap2, strength_th, count_th, bin_size_deg):
    """Return a list of (bin_lo, bin_hi, weighted_count) tuples covering [0, 90] degrees.

    weighted_count is the raw per-bin cell count multiplied by a normalised
    weighting: the pooled occupancy (mean of the two flowmaps' counts) of the
    cells in that bin, divided by the pooled occupancy of all thresholded
    cells in the heatmap. Cells failing --strength-th / --count-th are
    excluded entirely.
    """
    nrows = len(flowmap1)
    ncols = len(flowmap1[0])
    max_count1 = max_count(flowmap1)
    max_count2 = max_count(flowmap2)

    num_bins = math.ceil(90.0 / bin_size_deg)
    bin_counts = [0] * num_bins
    bin_occupancy = [0.0] * num_bins

    for r in range(nrows):
        for c in range(ncols):
            axis1, strength1, count1 = flowmap1[r][c]
            axis2, strength2, count2 = flowmap2[r][c]

            if not cell_is_valid(strength1, strength2, count1, count2, strength_th, count_th,
                                 max_count1, max_count2):
                continue

            delta_deg = math.degrees(angular_delta(axis1, axis2))
            bin_idx = min(int(delta_deg // bin_size_deg), num_bins - 1)
            bin_counts[bin_idx] += 1
            bin_occupancy[bin_idx] += (count1 + count2) / 2.0

    total_occupancy = sum(bin_occupancy)

    result = []
    for b in range(num_bins):
        bin_lo = b * bin_size_deg
        bin_hi = min((b + 1) * bin_size_deg, 90.0)
        weight = bin_occupancy[b] / total_occupancy if total_occupancy > 0 else 0.0
        weighted_count = bin_counts[b] * weight
        result.append((bin_lo, bin_hi, weighted_count))
    return result


def write_heatmap(cells, path):
    """Write a 2-D grid of floats as a plain CSV heatmap."""
    with open(path, 'w') as f:
        for row in cells:
            f.write(','.join(f"{value:.5f}" for value in row) + '\n')


def threshold_suffix(strength_th, count_th):
    """Return a filename-safe suffix encoding the thresholds, e.g. 'st-0p725-ct-0p400'."""
    strength_str = f"{strength_th:.3f}".replace('.', 'p')
    count_str = f"{count_th:.3f}".replace('.', 'p')
    return f"st-{strength_str}-ct-{count_str}"


def write_histogram(bins, path):
    """Write a list of (bin_lo, bin_hi, weighted_count) tuples as a plain CSV."""
    with open(path, 'w') as f:
        for bin_lo, bin_hi, weighted_count in bins:
            f.write(f"{bin_lo:.5f},{bin_hi:.5f},{weighted_count:.5f}\n")


def main():
    parser = argparse.ArgumentParser(
        description='Compute an axial angular-delta heatmap between two bee-movement flowmaps.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s run1-flowmap.csv run2-flowmap.csv
  %(prog)s run1-flowmap.csv run2-flowmap.csv --strength-th 0.3 --count-th 0.4 --basename cmp
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
        '--count-th', type=float, default=0.0, metavar='F',
        help='Minimum count fraction required of BOTH flowmap entries for a cell to be '
             'scored, where a count fraction is a count divided by the maximum count in '
             'its own flowmap (0.0-1.0). Cells where either entry falls below this are '
             'recorded as 0. Default: 0.0'
    )
    parser.add_argument(
        '--bin-size', type=float, default=15.0, metavar='D',
        help='Histogram bin size in degrees, spanning [0, 90]. Default: 15'
    )
    parser.add_argument(
        '--basename', default='output',
        help='Basename for the output CSV files (default: output)'
    )
    args = parser.parse_args()

    if not 0.0 <= args.strength_th <= 1.0:
        sys.exit(f"Error: --strength-th must be in range 0.0-1.0, got {args.strength_th}")
    if not 0.0 <= args.count_th <= 1.0:
        sys.exit(f"Error: --count-th must be in range 0.0-1.0, got {args.count_th}")
    if not 0.0 < args.bin_size <= 90.0:
        sys.exit(f"Error: --bin-size must be in range 0.0-90.0, got {args.bin_size}")

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

    suffix = threshold_suffix(args.strength_th, args.count_th)

    heatmap_path = f"{args.basename}-angdelta-heatmap-{suffix}.csv"
    write_heatmap(heatmap, heatmap_path)
    print(f"Wrote {heatmap_path}  (grid {cols1}x{rows1})")

    histogram = build_angdelta_histogram(
        flowmap1, flowmap2, args.strength_th, args.count_th, args.bin_size
    )
    histogram_path = f"{args.basename}-angdelta-histogram-{suffix}.csv"
    write_histogram(histogram, histogram_path)
    print(f"Wrote {histogram_path}  ({len(histogram)} bins of {args.bin_size} degrees)")


if __name__ == '__main__':
    main()
