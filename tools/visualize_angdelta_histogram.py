#!/usr/bin/env python3
"""
Visualise an angular-delta histogram from a CSV file.

The CSV file should contain one row per bin, as produced by
gen_angdelta_data.py's NAME-angdelta-histogram.csv output:

    bin_lo, bin_hi, weighted_count

The script draws a bar chart with one bar per bin (bin_lo to bin_hi on the
x-axis, weighted_count on the y-axis) and displays it on screen, with the
option to save it instead.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./visualize_angdelta_histogram.py <input_csv_file>
    ./visualize_angdelta_histogram.py <input_csv_file> --save-only
    ./visualize_angdelta_histogram.py <input_csv_file> --title "Run A vs Run B"
"""

import argparse
import os
import sys

import matplotlib.pyplot as plt
import numpy as np


def load_histogram(filename):
    """Load (bin_lo, bin_hi, weighted_count) rows from a CSV file."""
    bin_los, bin_his, weighted_counts = [], [], []
    try:
        with open(filename) as f:
            for lineno, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                parts = line.split(',')
                if len(parts) != 3:
                    print(f"Error: {filename}:{lineno}: expected 'bin_lo,bin_hi,weighted_count', "
                          f"got {line!r}", file=sys.stderr)
                    sys.exit(1)
                try:
                    bin_los.append(float(parts[0]))
                    bin_his.append(float(parts[1]))
                    weighted_counts.append(float(parts[2]))
                except ValueError as e:
                    print(f"Error: {filename}:{lineno}: {e}", file=sys.stderr)
                    sys.exit(1)
    except OSError as e:
        print(f"Error reading histogram file: {e}", file=sys.stderr)
        sys.exit(1)

    if not bin_los:
        print(f"Error: histogram file is empty: {filename}", file=sys.stderr)
        sys.exit(1)

    return np.array(bin_los), np.array(bin_his), np.array(weighted_counts)


def visualise_histogram(bin_los, bin_his, weighted_counts, title="Angular-Delta Histogram",
                        save_only=False, output_file=None):
    fig, ax = plt.subplots(figsize=(10, 6))

    widths = bin_his - bin_los
    ax.bar(bin_los, weighted_counts, width=widths, align='edge',
          color='steelblue', edgecolor='black', linewidth=0.5)

    ax.set_xlim(bin_los[0], bin_his[-1])
    ax.set_xticks(np.unique(np.concatenate([bin_los, bin_his])))

    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel('Angular Delta (degrees)', fontsize=12)
    ax.set_ylabel('Weighted Count', fontsize=12)
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5, axis='y')

    plt.tight_layout(pad=1.2)

    if save_only:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Histogram saved to: {output_file}")
        plt.close()
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Visualise an angular-delta histogram from a CSV file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s cmp-angdelta-histogram.csv                       # Display interactively
  %(prog)s cmp-angdelta-histogram.csv --save-only           # Save to .png without displaying
  %(prog)s cmp-angdelta-histogram.csv -t "Run A vs Run B"    # Custom title
        """
    )

    parser.add_argument('input_file',
                       help='Input CSV file containing bin_lo,bin_hi,weighted_count rows')
    parser.add_argument('--save-only',
                       action='store_true',
                       help='Save to PNG file without displaying on screen')
    parser.add_argument('--title', '-t',
                       metavar='TITLE',
                       help='Title for the plot (default: input filename without extension)')

    args = parser.parse_args()

    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    bin_los, bin_his, weighted_counts = load_histogram(args.input_file)

    basename = os.path.splitext(os.path.basename(args.input_file))[0]
    title = args.title if args.title else basename

    if args.save_only:
        output_file = os.path.splitext(args.input_file)[0] + '.png'
        visualise_histogram(bin_los, bin_his, weighted_counts, title=title,
                            save_only=True, output_file=output_file)
    else:
        visualise_histogram(bin_los, bin_his, weighted_counts, title=title, save_only=False)


if __name__ == '__main__':
    main()
