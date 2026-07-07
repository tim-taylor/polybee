#!/usr/bin/env python3
"""
Visualize a flowmap CSV file on its own, without an underlying heatmap raster.

A flowmap CSV (produced by Flowmap::print, e.g. the bee-movement flowmaps
written by the simulation, or the barrier flowmaps written by
gen_barrier_flowmap.py) encodes one row per grid row, with cells in
"axis:strength:count" format separated by commas. Each cell records the
dominant axis (radians), the strength of alignment to that axis (0-1), and a
count of observations (bee movements, or barriers, depending on how the
flowmap was generated).

This script draws that data directly as a grid of line segments (one per
non-empty cell, oriented along the cell's axis, thicker for higher relative
count and, by default, darker for higher strength of alignment) using the
same drawing logic as the _draw_flowmap() overlay in visualize_heatmap.py,
but with no heatmap image behind it. Pass --color/--colour to draw strength
of alignment using the polybee heatmap colour scale instead of grayscale,
with the scale shown as a colourbar on the right of the plot. Pass
--strength-th/--count-th (each in [0.0, 1.0], default 0.0) to only draw cells
whose strength / count fraction (count / max count) meets the given
threshold.

Optionally superimpose the tunnel outline, crop rows, and hive location from a
polybee config file, same as visualize_heatmap.py. When a config file is
supplied, axes are labelled in environment units and the flowmap cell size is
derived from env-width/env-height. Without a config file, axes are labelled in
flowmap grid-cell coordinates.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./visualize_flowmap.py -h
    ./visualize_flowmap.py <flowmap_csv_file>
    ./visualize_flowmap.py <flowmap_csv_file> --save-only
    ./visualize_flowmap.py <flowmap_csv_file> -c polybee.cfg
    ./visualize_flowmap.py <flowmap_csv_file> -c polybee.cfg -t "Barrier orientation"
    ./visualize_flowmap.py <flowmap_csv_file> --color
"""

import os
import sys
import argparse

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.cm import ScalarMappable
from matplotlib.colors import Normalize

from visualize_heatmap import (
    parse_config,
    load_flowmap,
    _draw_flowmap,
    _draw_tunnel,
    _draw_crop_rows,
    _draw_hive,
    create_polybee_colormap,
)


def visualize_flowmap(cells, fm_rows, fm_cols, title="Flowmap",
                      save_only=False, output_file=None,
                      env_width=None, env_height=None,
                      tunnel=None, entrances=None, crop_patches=None, hive=None,
                      use_color=False, strength_th=0.0, count_th=0.0):
    scale = min(10.0 / max(fm_rows, fm_cols), 0.7)  # inches per cell, capped so figure stays reasonable
    top_margin = 1.0     # title
    bottom_margin = 1.5  # x-axis tick labels + axis label
    fig_w = fm_cols * scale + 2.5
    fig_h = min(fm_rows * scale + top_margin + bottom_margin, 9.0)  # cap height for small screens
    _, ax = plt.subplots(figsize=(fig_w, fig_h))

    # Axis coordinates run top-to-bottom in increasing y, same convention as
    # the imshow(extent=...) used in visualize_heatmap.py, so flowmap row 0
    # is drawn at the top.
    if env_width is not None and env_height is not None:
        ax.set_xlim(0, env_width)
        ax.set_ylim(env_height, 0)
        ax.set_xticks(np.arange(0, env_width + 1, 50))
        ax.set_yticks(np.arange(0, env_height + 1, 50))
        xlabel, ylabel = 'X (env units)', 'Y (env units)'

        if crop_patches:
            _draw_crop_rows(ax, crop_patches)
        if tunnel:
            _draw_tunnel(ax, tunnel, entrances or [])
        if hive is not None:
            _draw_hive(ax, hive)
    else:
        ax.set_xlim(0, fm_cols)
        ax.set_ylim(fm_rows, 0)
        xlabel, ylabel = 'Flowmap column', 'Flowmap row'

    ax.set_aspect('equal')
    ax.set_facecolor('white')

    _draw_flowmap(ax, cells, fm_rows, fm_cols,
                  env_width=env_width, env_height=env_height,
                  heatmap_nrows=fm_rows, heatmap_ncols=fm_cols,
                  use_color=use_color, strength_th=strength_th, count_th=count_th)

    if use_color:
        sm = ScalarMappable(cmap=create_polybee_colormap(), norm=Normalize(vmin=0.0, vmax=1.0))
        sm.set_array([])
        cbar = plt.colorbar(sm, ax=ax)
        cbar.set_label('Strength of alignment', rotation=270, labelpad=20)

    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)

    plt.tight_layout(pad=1.2)

    if save_only:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Flowmap saved to: {output_file}")
        plt.close()
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Visualize a flowmap CSV file (from Flowmap::print) on its own, '
                    'without an underlying heatmap raster.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s flowmap.csv                          # Display flowmap interactively
  %(prog)s flowmap.csv --save-only              # Save to flowmap.png without displaying
  %(prog)s flowmap.csv -c polybee.cfg           # Overlay tunnel, crop rows, hive
        """
    )

    parser.add_argument('input_file',
                       help='Input flowmap CSV file (axis:strength:count cells, from Flowmap::print)')
    parser.add_argument('--save-only',
                       action='store_true',
                       help='Save to PNG file without displaying on screen')
    parser.add_argument('--config', '-c',
                       metavar='CFG',
                       help='Polybee config file; if supplied, axes are labelled in env units '
                            'and tunnel, crop rows, and hive are drawn as overlays')
    parser.add_argument('--title', '-t',
                       metavar='TITLE',
                       help='Title for the plot (default: input filename without extension)')
    parser.add_argument('--color', '--colour',
                       action='store_true',
                       help='Colour lines by strength of alignment using the polybee heatmap '
                            'colour scale (instead of grayscale), with a colourbar on the right')
    parser.add_argument('--strength-th',
                       type=float, default=0.0, metavar='TH',
                       help='Only draw cells whose strength is >= TH, in range [0.0, 1.0] (default: 0.0)')
    parser.add_argument('--count-th',
                       type=float, default=0.0, metavar='TH',
                       help='Only draw cells whose count fraction (count / max count) is >= TH, '
                            'in range [0.0, 1.0] (default: 0.0)')

    args = parser.parse_args()

    for name, value in (('--strength-th', args.strength_th), ('--count-th', args.count_th)):
        if not 0.0 <= value <= 1.0:
            print(f"Error: {name} must be in range [0.0, 1.0], got {value}", file=sys.stderr)
            sys.exit(1)

    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    cells, fm_rows, fm_cols = load_flowmap(args.input_file)

    env_width, env_height = None, None
    tunnel, entrances, crop_patches, hive = {}, [], [], None
    if args.config:
        env_width, env_height, tunnel, entrances, crop_patches, hive = parse_config(args.config)

    basename = os.path.splitext(os.path.basename(args.input_file))[0]
    title = args.title if args.title else basename

    if args.save_only:
        output_file = os.path.splitext(args.input_file)[0] + '.png'
        visualize_flowmap(cells, fm_rows, fm_cols, title=title, save_only=True, output_file=output_file,
                          env_width=env_width, env_height=env_height,
                          tunnel=tunnel, entrances=entrances,
                          crop_patches=crop_patches, hive=hive,
                          use_color=args.color, strength_th=args.strength_th, count_th=args.count_th)
    else:
        visualize_flowmap(cells, fm_rows, fm_cols, title=title, save_only=False,
                          env_width=env_width, env_height=env_height,
                          tunnel=tunnel, entrances=entrances,
                          crop_patches=crop_patches, hive=hive,
                          use_color=args.color, strength_th=args.strength_th, count_th=args.count_th)


if __name__ == '__main__':
    main()
