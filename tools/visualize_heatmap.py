#!/usr/bin/env python3
"""
Visualize a normalized heatmap from a CSV file.

The CSV file should contain numeric values representing a normalized heatmap.
The script displays the heatmap on screen with the option to save it.
Use --save-only to skip the display and save directly to a PNG file.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./visualize_heatmap.py <input_csv_file>
    ./visualize_heatmap.py <input_csv_file> --save-only
"""

import matplotlib.pyplot as plt
import numpy as np
import sys
import argparse
import os
from matplotlib.colors import LinearSegmentedColormap

def create_polybee_colormap():
    """
    Create a custom colormap matching the one used in LocalVis.cpp.

    Blue (0) -> Cyan (0.25) -> Green (0.5) -> Yellow (0.75) -> Red (1.0)
    """
    colors = [
        (0.0, (0.0, 0.0, 1.0)),      # Blue at 0.0
        (0.25, (0.0, 1.0, 1.0)),     # Cyan at 0.25
        (0.5, (0.0, 1.0, 0.0)),      # Green at 0.5
        (0.75, (1.0, 1.0, 0.0)),     # Yellow at 0.75
        (1.0, (1.0, 0.0, 0.0))       # Red at 1.0
    ]

    return LinearSegmentedColormap.from_list('polybee',
                                            [c for _, c in colors],
                                            N=256)

def load_heatmap(filename):
    """Load heatmap data from CSV file."""
    try:
        data = np.loadtxt(filename, delimiter=',')
        return data
    except Exception as e:
        print(f"Error loading CSV file: {e}", file=sys.stderr)
        sys.exit(1)

def visualize_heatmap(data, title="Heatmap", save_only=False, output_file=None):
    """
    Visualize the heatmap data.

    Args:
        data: 2D numpy array containing the heatmap values
        title: Title for the plot
        save_only: If True, save to file without displaying
        output_file: Output filename (only used if save_only is True)
    """
    fig, ax = plt.subplots(figsize=(10, 8))

    # Create custom colormap matching LocalVis.cpp
    cmap = create_polybee_colormap()

    # Create heatmap with custom blue-to-red colormap
    im = ax.imshow(data, cmap=cmap, interpolation='nearest', aspect='auto')

    # Add colorbar
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Normalized Value', rotation=270, labelpad=20)

    # Set title and labels
    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel('X coordinate', fontsize=12)
    ax.set_ylabel('Y coordinate', fontsize=12)

    # Add grid for better readability
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)

    # Add text annotations for each cell (optional, only for small heatmaps)
    height, width = data.shape
    if height <= 20 and width <= 20:  # Only annotate if heatmap is small enough
        for i in range(height):
            for j in range(width):
                text = ax.text(j, i, f'{data[i, j]:.2f}',
                             ha="center", va="center", color="black", fontsize=8)

    plt.tight_layout()

    if save_only:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Heatmap saved to: {output_file}")
        plt.close()
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(
        description='Visualize a normalized heatmap from a CSV file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s heatmap.csv                    # Display heatmap interactively
  %(prog)s heatmap.csv --save-only        # Save to heatmap.png without displaying
        """
    )

    parser.add_argument('input_file',
                       help='Input CSV file containing normalized heatmap data')
    parser.add_argument('--save-only',
                       action='store_true',
                       help='Save to PNG file without displaying on screen')

    args = parser.parse_args()

    # Check if input file exists
    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    # Load the heatmap data
    data = load_heatmap(args.input_file)

    # Get the basename without extension for the title and output file
    basename = os.path.splitext(os.path.basename(args.input_file))[0]

    if args.save_only:
        # Generate output filename: replace .csv with .png
        output_file = os.path.splitext(args.input_file)[0] + '.png'
        visualize_heatmap(data, title=basename, save_only=True, output_file=output_file)
    else:
        visualize_heatmap(data, title=basename, save_only=False)

if __name__ == '__main__':
    main()
