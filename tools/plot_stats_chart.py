#!/usr/bin/env python3
"""
Plot statistics chart from CSV data showing Q1, Median, Q3, and Mean values.

The script reads CSV data with columns: Reps, Q1, Median, Q3, Mean
and creates a visualization showing how these statistics vary across different
values of Reps (repetitions).

Dependencies:
    pip install matplotlib numpy pandas

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy python3-pandas

Usage:
    ./plot_stats_chart.py <input_csv_file>
    ./plot_stats_chart.py <input_csv_file> --save <output.png>
"""

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys
import argparse
import os

def load_stats_data(filename):
    """
    Load statistics data from CSV file.

    Args:
        filename: Path to the CSV file

    Returns:
        Tuple of (pandas DataFrame, x_column_name)
        where x_column_name is either 'Reps' or 'NumBees'
    """
    try:
        df = pd.read_csv(filename)

        # Determine which type of file this is (Reps or NumBees)
        x_column = None
        if 'Reps' in df.columns:
            x_column = 'Reps'
        elif 'NumBees' in df.columns:
            x_column = 'NumBees'
        else:
            print("Error: CSV must have either 'Reps' or 'NumBees' as first column", file=sys.stderr)
            sys.exit(1)

        # Check that required statistic columns exist
        required_cols = ['Q1', 'Median', 'Q3', 'Mean']
        for col in required_cols:
            if col not in df.columns:
                print(f"Error: Required column '{col}' not found in CSV file", file=sys.stderr)
                sys.exit(1)

        return df, x_column
    except Exception as e:
        print(f"Error loading CSV file: {e}", file=sys.stderr)
        sys.exit(1)

def create_stats_plot(df, x_column, title="Statistics Chart", save_file=None):
    """
    Create a visualization of the statistics data.

    Args:
        df: pandas DataFrame with columns (Reps or NumBees), Q1, Median, Q3, Mean
        x_column: Name of the x-axis column ('Reps' or 'NumBees')
        title: Title for the plot
        save_file: If provided, save to this file instead of displaying
    """
    fig, ax = plt.subplots(figsize=(12, 8))

    # Extract data
    x_values = df[x_column].values
    q1 = df['Q1'].values
    median = df['Median'].values
    q3 = df['Q3'].values
    mean = df['Mean'].values

    # Create shaded region for interquartile range (Q1 to Q3)
    ax.fill_between(x_values, q1, q3, alpha=0.3, color='lightblue',
                     label='Interquartile Range (Q1-Q3)')

    # Plot lines for each statistic
    ax.plot(x_values, q1, 'o-', color='blue', linewidth=2, markersize=6,
            label='Q1 (25th percentile)', alpha=0.7)
    ax.plot(x_values, median, 's-', color='red', linewidth=2.5, markersize=7,
            label='Median (50th percentile)', alpha=0.9)
    ax.plot(x_values, q3, '^-', color='blue', linewidth=2, markersize=6,
            label='Q3 (75th percentile)', alpha=0.7)
    ax.plot(x_values, mean, 'd-', color='green', linewidth=2, markersize=6,
            label='Mean', alpha=0.8)

    # Add value labels at each point (for median and mean)
    for i, (x, y_median, y_mean) in enumerate(zip(x_values, median, mean)):
        ax.text(x, y_median, f'{y_median:.3f}', fontsize=7,
                ha='right', va='bottom', color='red')
        ax.text(x, y_mean, f'{y_mean:.3f}', fontsize=7,
                ha='left', va='top', color='green')

    # Customize the plot with appropriate x-axis label
    x_label = 'Number of Repetitions' if x_column == 'Reps' else 'Number of Bees'
    ax.set_xlabel(x_label, fontsize=12, fontweight='bold')
    ax.set_ylabel('Value', fontsize=12, fontweight='bold')
    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.legend(loc='best', framealpha=0.9, fontsize=10)
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)

    # Set x-axis to show all values
    ax.set_xticks(x_values)

    # Add some statistics as text box
    stats_text = f'Data Points: {len(x_values)}\n'
    stats_text += f'{x_column} Range: {x_values.min()}-{x_values.max()}\n'
    stats_text += f'Median Range: {median.min():.4f}-{median.max():.4f}\n'
    stats_text += f'Mean Range: {mean.min():.4f}-{mean.max():.4f}'

    ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
            verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
            fontsize=9, family='monospace')

    plt.tight_layout()

    if save_file:
        plt.savefig(save_file, dpi=150, bbox_inches='tight')
        print(f"Chart saved to: {save_file}")
        plt.close()
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(
        description='Plot statistics chart from CSV data.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s data.csv                           # Display chart interactively
  %(prog)s data.csv --save output.png         # Save to PNG file
  %(prog)s data.csv --save output.pdf         # Save to PDF file
        """
    )

    parser.add_argument('input_file',
                       help='Input CSV file with columns: (Reps or NumBees), Q1, Median, Q3, Mean')
    parser.add_argument('--save',
                       help='Save chart to file instead of displaying (PNG, PDF, etc.)')

    args = parser.parse_args()

    # Check if input file exists
    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    # Load the data
    print(f"Loading data from {args.input_file}...")
    df, x_column = load_stats_data(args.input_file)
    print(f"Loaded {len(df)} data points (x-axis: {x_column})")

    # Get the basename for the title
    basename = os.path.splitext(os.path.basename(args.input_file))[0]

    # Create the plot
    create_stats_plot(df, x_column, title=basename, save_file=args.save)

    # Print summary
    print("\nData Summary:")
    print(df.to_string(index=False))

if __name__ == '__main__':
    main()
