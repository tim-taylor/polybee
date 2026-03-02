#!/usr/bin/env python3
"""
Create a box and whisker plot from numeric data in one or more CSV files.

The script reads all numeric values from each CSV file and creates a single
figure with one box plot per file, labelled by filename (or custom labels).

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./plot_boxplot.py <file1.csv> [file2.csv ...]
    ./plot_boxplot.py <file1.csv> [file2.csv ...] --save-only
    ./plot_boxplot.py <file1.csv> [file2.csv ...] --title "My Title" --ylabel "Fitness" --labels "A" "B"
"""

import matplotlib.pyplot as plt
import numpy as np
import sys
import argparse
import os

def load_csv_data(filename):
    """
    Load all numeric values from a CSV file.

    Args:
        filename: Path to the CSV file

    Returns:
        1D numpy array containing all numeric values found in the file
    """
    try:
        data = np.loadtxt(filename, delimiter=',')
        if data.ndim > 1:
            values = data.flatten()
        else:
            values = data
        return values

    except Exception as e:
        print(f"Error loading CSV file '{filename}': {e}", file=sys.stderr)
        sys.exit(1)

def create_boxplot(datasets, labels, title="", ylabel="Value", save_only=False, output_file=None):
    """
    Create a box and whisker plot with one box per dataset.

    Args:
        datasets: List of 1D numpy arrays
        labels: List of x-axis tick labels (one per dataset)
        title: Title for the figure
        ylabel: Label for the vertical axis
        save_only: If True, save to file without displaying
        output_file: Output filename (only used if save_only is True)
    """
    fig, ax = plt.subplots(figsize=(max(8, 3 * len(datasets)), 8))

    bp = ax.boxplot(datasets, vert=True, patch_artist=True, widths=0.5)

    for box in bp['boxes']:
        box.set_facecolor('lightblue')
        box.set_edgecolor('darkblue')
        box.set_linewidth(1.5)

    for whisker in bp['whiskers']:
        whisker.set_color('darkblue')
        whisker.set_linewidth(1.5)

    for cap in bp['caps']:
        cap.set_color('darkblue')
        cap.set_linewidth(1.5)

    for median in bp['medians']:
        median.set_color('red')
        median.set_linewidth(2)

    for flier in bp['fliers']:
        flier.set_marker('o')
        flier.set_markerfacecolor('red')
        flier.set_markeredgecolor('darkred')
        flier.set_markersize(6)
        flier.set_alpha(0.5)

    if title:
        ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_xticklabels(labels, rotation=15, ha='right', fontsize=10)

    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5, axis='y')

    # Show stats text box only for a single dataset (too crowded otherwise)
    if len(datasets) == 1:
        data = datasets[0]
        stats_text = (
            f'Count: {len(data)}\n'
            f'Min: {np.min(data):.4f}\n'
            f'Q1: {np.percentile(data, 25):.4f}\n'
            f'Median: {np.median(data):.4f}\n'
            f'Q3: {np.percentile(data, 75):.4f}\n'
            f'Max: {np.max(data):.4f}\n'
            f'Mean: {np.mean(data):.4f}\n'
            f'Std Dev: {np.std(data):.4f}'
        )
        ax.text(0.98, 0.98, stats_text, transform=ax.transAxes,
                verticalalignment='top', horizontalalignment='right',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
                fontsize=10, family='monospace')

    plt.tight_layout()

    if save_only:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Box plot saved to: {output_file}")
        plt.close()
    else:
        plt.show()

def print_stats(label, data):
    print(f"\n{label}:")
    print(f"  Count:  {len(data)}")
    print(f"  Min:    {np.min(data):.4f}")
    print(f"  Q1:     {np.percentile(data, 25):.4f}")
    print(f"  Median: {np.median(data):.4f}")
    print(f"  Q3:     {np.percentile(data, 75):.4f}")
    print(f"  Max:    {np.max(data):.4f}")
    print(f"  Mean:   {np.mean(data):.4f}")
    print(f"  Std Dev:{np.std(data):.4f}")

def main():
    parser = argparse.ArgumentParser(
        description='Create a box and whisker plot from numeric data in one or more CSV files.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s data.csv                              # Single file, interactive
  %(prog)s a.csv b.csv c.csv                    # Multiple files, interactive
  %(prog)s a.csv b.csv --save-only              # Save to PNG without displaying
  %(prog)s a.csv b.csv --title "Results" --ylabel "Fitness"
  %(prog)s a.csv b.csv --labels "Control" "Treatment"
        """
    )

    parser.add_argument('input_files', nargs='+',
                        help='One or more input CSV files containing numeric data')
    parser.add_argument('--save-only', action='store_true',
                        help='Save to PNG file without displaying on screen')
    parser.add_argument('--title', default='',
                        help='Title for the figure')
    parser.add_argument('--ylabel', default='Value',
                        help='Label for the vertical axis (default: "Value")')
    parser.add_argument('--labels', nargs='+', metavar='LABEL',
                        help='Custom labels for each input file (must match number of files)')

    args = parser.parse_args()

    # Validate files
    for f in args.input_files:
        if not os.path.isfile(f):
            print(f"Error: File not found: {f}", file=sys.stderr)
            sys.exit(1)

    # Validate custom labels count
    if args.labels is not None and len(args.labels) != len(args.input_files):
        print(
            f"Error: --labels count ({len(args.labels)}) must match number of input files ({len(args.input_files)})",
            file=sys.stderr
        )
        sys.exit(1)

    # Load data from each file
    datasets = []
    for f in args.input_files:
        data = load_csv_data(f)
        if len(data) == 0:
            print(f"Error: No numeric data found in file: {f}", file=sys.stderr)
            sys.exit(1)
        datasets.append(data)

    # Build x-axis labels
    if args.labels:
        x_labels = args.labels
    else:
        x_labels = [os.path.basename(f) for f in args.input_files]

    # Print statistics to console
    for label, data in zip(x_labels, datasets):
        print_stats(label, data)

    if args.save_only:
        # Base output name on first input file
        output_file = os.path.splitext(args.input_files[0])[0] + '.png'
        create_boxplot(datasets, x_labels, title=args.title, ylabel=args.ylabel,
                       save_only=True, output_file=output_file)
    else:
        create_boxplot(datasets, x_labels, title=args.title, ylabel=args.ylabel,
                       save_only=False)

if __name__ == '__main__':
    main()
