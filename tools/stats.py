#!/usr/bin/env python3
"""Calculate mean, median, and standard deviation from files of numbers."""

import argparse
import sys
import statistics
from pathlib import Path

try:
    from scipy import stats as scipy_stats
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False

try:
    import matplotlib.pyplot as plt
    import numpy as np
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False


def load_file(filename):
    """Load numbers from a file. Returns list of numbers or None on error."""
    try:
        with open(filename) as f:
            numbers = [float(line.strip()) for line in f if line.strip()]
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found", file=sys.stderr)
        return None
    except ValueError as e:
        print(f"Error: Invalid number in file '{filename}': {e}", file=sys.stderr)
        return None

    if not numbers:
        print(f"Error: No numbers found in file '{filename}'", file=sys.stderr)
        return None

    return numbers


def print_file_stats(filename, numbers):
    """Print statistics for a single file."""
    mean = statistics.mean(numbers)
    median = statistics.median(numbers)
    stdev = statistics.stdev(numbers) if len(numbers) > 1 else 0.0

    print(f"File:   {filename}")
    print(f"Count:  {len(numbers)}")
    print(f"Mean:   {mean}")
    print(f"Median: {median}")
    print(f"StdDev: {stdev}")


def print_significance_matrix(filenames, data, alpha=0.05):
    """Print NxN matrix showing statistical significance of median differences."""
    if not SCIPY_AVAILABLE:
        print("\nNote: Install scipy for significance matrix (pip install scipy)",
              file=sys.stderr)
        return

    n = len(filenames)
    if n < 2:
        return

    print("\n" + "=" * 60)
    print("Median Difference Significance Matrix (Mann-Whitney U test)")
    print(f"Alpha = {alpha}, '*' = significant difference")
    print("=" * 60)

    # Create short labels for files
    labels = [Path(f).stem for f in filenames]
    # Ensure unique labels by adding index if needed
    if len(set(labels)) != len(labels):
        labels = [f"F{i}" for i in range(n)]

    # Calculate max label width for formatting
    max_label = max(len(label) for label in labels)
    col_width = max(max_label, 5)

    # Print header row
    header = " " * (max_label + 2)
    for label in labels:
        header += f"{label:>{col_width}} "
    print(header)

    # Print matrix rows
    for i, label_i in enumerate(labels):
        row = f"{label_i:>{max_label}}: "
        for j in range(n):
            if i == j:
                cell = "-"
            else:
                _, p_value = scipy_stats.mannwhitneyu(
                    data[i], data[j], alternative='two-sided'
                )
                cell = "*" if p_value < alpha else "."
            row += f"{cell:>{col_width}} "
        print(row)

    print("\nP-values:")
    for i in range(n):
        for j in range(i + 1, n):
            _, p_value = scipy_stats.mannwhitneyu(
                data[i], data[j], alternative='two-sided'
            )
            sig = " (significant)" if p_value < alpha else ""
            print(f"  {labels[i]} vs {labels[j]}: p = {p_value:.6f}{sig}")


def plot_statistics(filenames, data, use_means=False, ylabel=None, title=None):
    """Plot a bar chart of median (or mean) with std dev whiskers for each file."""
    if not MATPLOTLIB_AVAILABLE:
        print("\nNote: Install matplotlib for plotting (pip install matplotlib numpy)",
              file=sys.stderr)
        return

    # Calculate statistics for each file
    values = [statistics.mean(d) for d in data] if use_means else \
             [statistics.median(d) for d in data]
    stdevs = [statistics.stdev(d) if len(d) > 1 else 0.0 for d in data]
    stat_label = 'Mean' if use_means else 'Median'

    # Create short labels for files
    labels = [Path(f).stem for f in filenames]
    if len(set(labels)) != len(labels):
        labels = [f"F{i}" for i in range(len(filenames))]

    x = np.arange(len(labels))
    width = 0.5

    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.5), 6))

    bars = ax.bar(x, values, width, label=stat_label, color='steelblue',
                  yerr=stdevs, capsize=5, error_kw={'color': 'black', 'linewidth': 1.5})

    ax.set_ylabel(ylabel if ylabel else stat_label)
    ax.set_title(title if title else f'{stat_label} by File')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=45, ha='right')
    ax.legend()

    # Add value labels on bars (above the whiskers)
    for bar, val, sd in zip(bars, values, stdevs):
        ax.annotate(f'{val:.3f}',
                    xy=(bar.get_x() + bar.get_width() / 2, val + sd),
                    xytext=(0, 5),
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=8)

    # Add extra headroom above the highest whisker + label
    max_top = max(v + s for v, s in zip(values, stdevs))
    ax.set_ylim(top=max_top * 1.15 if max_top > 0 else 1.0)

    plt.tight_layout()
    plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Calculate mean, median, and standard deviation from files of numbers.'
    )
    parser.add_argument('files', nargs='+', metavar='FILE',
                        help='Input file(s) containing numbers, one per line')
    parser.add_argument('-g', '--graph', action='store_true',
                        help='Display a graph of the statistics')
    parser.add_argument('-m', '--means', action='store_true',
                        help='Show means instead of medians in the graph')
    parser.add_argument('--ylabel', type=str, default=None,
                        help='Label for the vertical axis')
    parser.add_argument('--title', type=str, default=None,
                        help='Title for the plot')
    args = parser.parse_args()

    valid_filenames = []
    all_data = []

    # Load and print stats for each file
    for i, filename in enumerate(args.files):
        if i > 0:
            print()  # Blank line between files
        numbers = load_file(filename)
        if numbers is not None:
            print_file_stats(filename, numbers)
            valid_filenames.append(filename)
            all_data.append(numbers)

    if not valid_filenames:
        sys.exit(1)

    # Print significance matrix if multiple files
    if len(valid_filenames) >= 2:
        print_significance_matrix(valid_filenames, all_data)

    # Plot statistics if requested
    if args.graph:
        plot_statistics(valid_filenames, all_data, use_means=args.means,
                       ylabel=args.ylabel, title=args.title)


if __name__ == "__main__":
    main()
