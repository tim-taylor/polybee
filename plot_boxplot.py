#!/usr/bin/env python3
"""
Create a box and whisker plot from numeric data in a CSV file.

The script reads all numeric values from a CSV file and creates a box plot
showing the distribution of the data including median, quartiles, and outliers.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./plot_boxplot.py <input_csv_file>
    ./plot_boxplot.py <input_csv_file> --save-only
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
        # Load the entire CSV as a 2D array
        data = np.loadtxt(filename, delimiter=',')

        # Flatten to 1D array to get all values
        if data.ndim > 1:
            values = data.flatten()
        else:
            values = data

        return values

    except Exception as e:
        print(f"Error loading CSV file: {e}", file=sys.stderr)
        sys.exit(1)

def create_boxplot(data, title="Box and Whisker Plot", save_only=False, output_file=None):
    """
    Create a box and whisker plot from the data.

    Args:
        data: 1D numpy array containing the numeric values
        title: Title for the plot
        save_only: If True, save to file without displaying
        output_file: Output filename (only used if save_only is True)
    """
    fig, ax = plt.subplots(figsize=(10, 8))

    # Create box plot
    bp = ax.boxplot(data, vert=True, patch_artist=True, widths=0.5)

    # Customize box plot appearance
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

    # Set title and labels
    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_ylabel('Value', fontsize=12)
    ax.set_xticklabels(['Data'])

    # Add grid for better readability
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5, axis='y')

    # Add statistics text box
    stats_text = f'Count: {len(data)}\n'
    stats_text += f'Min: {np.min(data):.4f}\n'
    stats_text += f'Q1: {np.percentile(data, 25):.4f}\n'
    stats_text += f'Median: {np.median(data):.4f}\n'
    stats_text += f'Q3: {np.percentile(data, 75):.4f}\n'
    stats_text += f'Max: {np.max(data):.4f}\n'
    stats_text += f'Mean: {np.mean(data):.4f}\n'
    stats_text += f'Std Dev: {np.std(data):.4f}'

    # Position text box in upper right
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

def main():
    parser = argparse.ArgumentParser(
        description='Create a box and whisker plot from numeric data in a CSV file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s data.csv                    # Display box plot interactively
  %(prog)s data.csv --save-only        # Save to data.png without displaying
        """
    )

    parser.add_argument('input_file',
                       help='Input CSV file containing numeric data')
    parser.add_argument('--save-only',
                       action='store_true',
                       help='Save to PNG file without displaying on screen')

    args = parser.parse_args()

    # Check if input file exists
    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    # Load the data
    data = load_csv_data(args.input_file)

    if len(data) == 0:
        print(f"Error: No numeric data found in file", file=sys.stderr)
        sys.exit(1)

    # Get the basename without extension for the title
    basename = os.path.splitext(os.path.basename(args.input_file))[0]

    if args.save_only:
        # Generate output filename: replace .csv with .png
        output_file = os.path.splitext(args.input_file)[0] + '.png'
        create_boxplot(data, title=basename, save_only=True, output_file=output_file)
    else:
        create_boxplot(data, title=basename, save_only=False)

    # Print summary statistics to console
    print("\nData Statistics:")
    print(f"  Count: {len(data)}")
    print(f"  Min: {np.min(data):.4f}")
    print(f"  Q1 (25th percentile): {np.percentile(data, 25):.4f}")
    print(f"  Median (50th percentile): {np.median(data):.4f}")
    print(f"  Q3 (75th percentile): {np.percentile(data, 75):.4f}")
    print(f"  Max: {np.max(data):.4f}")
    print(f"  Mean: {np.mean(data):.4f}")
    print(f"  Std Dev: {np.std(data):.4f}")

if __name__ == '__main__':
    main()
