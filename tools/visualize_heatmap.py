#!/usr/bin/env python3
"""
Visualize a heatmap from a CSV file.

The CSV file should contain a 2D grid of numeric values (normalized or not
-- e.g. raw counts, or an angular-delta heatmap from gen_angdelta-heatmap.py).
The script displays the heatmap on screen with the option to save it.
Use --save-only to skip the display and save directly to a PNG file.

By default the colour scale auto-scales to the min/max values found in the
CSV. Pass --color-scale-max N to fix the colour scale to run from 0 to N
instead -- useful when you want multiple heatmaps rendered on the same
scale, or when the data has a known theoretical maximum (e.g. pi/2 for
angular deltas).

Optionally superimpose a flowmap (produced by Flowmap::print) over the heatmap.
The flowmap CSV contains cells in "axis:strength:count" format and is assumed to
cover the same environment dimensions as the heatmap (cell sizes may differ).

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./visualize_heatmap.py <input_csv_file>
    ./visualize_heatmap.py <input_csv_file> --save-only
    ./visualize_heatmap.py <input_csv_file> -c polybee.cfg
    ./visualize_heatmap.py <input_csv_file> -c polybee.cfg -f flowmap.csv
    ./visualize_heatmap.py <input_csv_file> --color-scale-max 1.5708
"""

import math
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import numpy as np
import sys
import argparse
import os
from matplotlib.collections import LineCollection
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


def parse_config(config_file):
    """Read env dimensions and overlay elements from a polybee config file."""
    env_width = None
    env_height = None
    tunnel = {}
    entrances = []   # list of {'e1', 'e2', 'side'}
    crop_patches = []  # list of {'x','y','w','h','repeat','spacing','direction'}
    hive = None      # (x, y, direction)

    try:
        with open(config_file) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#') or '=' not in line:
                    continue
                key, _, value = line.partition('=')
                key = key.strip()
                value = value.strip()

                if key == 'env-width':
                    env_width = float(value)
                elif key == 'env-height':
                    env_height = float(value)
                elif key == 'tunnel-x':
                    tunnel['x'] = float(value)
                elif key == 'tunnel-y':
                    tunnel['y'] = float(value)
                elif key == 'tunnel-width':
                    tunnel['width'] = float(value)
                elif key == 'tunnel-height':
                    tunnel['height'] = float(value)
                elif key == 'tunnel-entrance':
                    # Format: e1,e2:side[:nettype]
                    # e1,e2 are offsets from the tunnel corner along the specified side.
                    parts = value.split(':')
                    if len(parts) >= 2:
                        ab = parts[0].split(',')
                        if len(ab) >= 2:
                            try:
                                entrances.append({
                                    'e1': float(ab[0]),
                                    'e2': float(ab[1]),
                                    'side': int(parts[1]),
                                })
                            except ValueError:
                                pass
                elif key == 'patch':
                    # Format: x,y,w,h:type:p2:p3:repeat:spacing,direction:flag
                    # Draw crop rows when repeat (8th param) > 1 and spacing (9th param) > 0.
                    parts = value.split(':')
                    if len(parts) < 6:
                        continue
                    geom = parts[0].split(',')
                    if len(geom) < 4:
                        continue
                    try:
                        x, y, w, h = (float(geom[0]), float(geom[1]),
                                      float(geom[2]), float(geom[3]))
                        repeat = float(parts[4])
                        spacing_dir = parts[5].split(',')
                        spacing = float(spacing_dir[0])
                        direction = int(spacing_dir[1]) if len(spacing_dir) > 1 else 0
                    except ValueError:
                        continue
                    if repeat > 1 and spacing > 0:
                        crop_patches.append({
                            'x': x, 'y': y, 'w': w, 'h': h,
                            'repeat': int(repeat),
                            'spacing': spacing,
                            'direction': direction,
                        })
                elif key == 'hive':
                    # Format: x,y:direction
                    parts = value.split(':')
                    if len(parts) >= 2:
                        xy = parts[0].split(',')
                        if len(xy) >= 2:
                            try:
                                hive = (float(xy[0]), float(xy[1]), int(parts[1]))
                            except ValueError:
                                pass
    except OSError as e:
        print(f"Error reading config file: {e}", file=sys.stderr)
        sys.exit(1)

    if env_width is None or env_height is None:
        print("Error: env-width or env-height not found in config file", file=sys.stderr)
        sys.exit(1)

    return env_width, env_height, tunnel, entrances, crop_patches, hive


def _draw_tunnel(ax, tunnel, entrances):
    """Draw the tunnel outline with entrance gaps."""
    if not all(k in tunnel for k in ('x', 'y', 'width', 'height')):
        return
    tx, ty = tunnel['x'], tunnel['y']
    tw, th = tunnel['width'], tunnel['height']

    # Each entry: (fixed_coord, range_start, range_end, horizontal, side_index)
    # For North/South (horizontal=True), e1/e2 are x-offsets from tunnel_x.
    # For East/West  (horizontal=False), e1/e2 are y-offsets from tunnel_y.
    walls = [
        (ty,      tx, tx + tw, True,  0),  # North (top)
        (tx + tw, ty, ty + th, False, 1),  # East (right)
        (ty + th, tx, tx + tw, True,  2),  # South (bottom)
        (tx,      ty, ty + th, False, 3),  # West (left)
    ]

    for fixed, start, end, horizontal, side_idx in walls:
        origin = tx if horizontal else ty
        gaps = sorted(
            (origin + e['e1'], origin + e['e2'])
            for e in entrances if e['side'] == side_idx
        )
        pos = start
        for gap_a, gap_b in gaps:
            if pos < gap_a:
                pts = ([pos, gap_a], [fixed, fixed]) if horizontal else ([fixed, fixed], [pos, gap_a])
                ax.plot(*pts, color='black', lw=2, zorder=3)
            pos = max(pos, gap_b)
        if pos < end:
            pts = ([pos, end], [fixed, fixed]) if horizontal else ([fixed, fixed], [pos, end])
            ax.plot(*pts, color='black', lw=2, zorder=3)


def _draw_crop_rows(ax, crop_patches):
    """Draw semi-transparent rectangles for each crop row repeat."""
    for patch in crop_patches:
        x, y, w, h = patch['x'], patch['y'], patch['w'], patch['h']
        for i in range(patch['repeat']):
            xi = x + i * patch['spacing'] if patch['direction'] == 0 else x
            yi = y if patch['direction'] == 0 else y + i * patch['spacing']
            ax.add_patch(mpatches.Rectangle(
                (xi, yi), w, h,
                linewidth=2, edgecolor='#a8a8a8', facecolor='none', zorder=2,
            ))


def _draw_hive(ax, hive):
    """Draw a small labelled square at the hive midpoint."""
    hx, hy, _ = hive
    size = 20  # env units
    ax.add_patch(mpatches.Rectangle(
        (hx - size / 2, hy - size / 2), size, size,
        linewidth=2, edgecolor='#e8e8e8', facecolor='none', zorder=4,
    ))


def load_flowmap(filename):
    """
    Load a flowmap CSV produced by Flowmap::print.

    Returns (cells, fm_rows, fm_cols) where cells is a 2-D list indexed
    [row][col], each entry a (axis, strength, count) tuple.
    """
    cells = []
    try:
        with open(filename) as f:
            for lineno, line in enumerate(f, 1):
                line = line.rstrip('\n')
                if not line:
                    continue
                row = []
                for token in line.split(','):
                    parts = token.split(':')
                    if len(parts) != 3:
                        print(f"Error: {filename}:{lineno}: expected 'axis:strength:count', got {token!r}",
                              file=sys.stderr)
                        sys.exit(1)
                    try:
                        row.append((float(parts[0]), float(parts[1]), int(parts[2])))
                    except ValueError as e:
                        print(f"Error: {filename}:{lineno}: {e}", file=sys.stderr)
                        sys.exit(1)
                cells.append(row)
    except OSError as e:
        print(f"Error reading flowmap file: {e}", file=sys.stderr)
        sys.exit(1)

    if not cells:
        print(f"Error: flowmap file is empty: {filename}", file=sys.stderr)
        sys.exit(1)

    fm_rows = len(cells)
    fm_cols = len(cells[0])
    return cells, fm_rows, fm_cols


def _draw_flowmap(ax, cells, fm_rows, fm_cols,
                  env_width=None, env_height=None,
                  heatmap_nrows=None, heatmap_ncols=None,
                  use_color=False, strength_th=0.0, count_th=0.0):
    """
    Superimpose flowmap lines over the heatmap axes using the same logic as
    LocalVis::drawFlowmap().

    Axes are in env units when env_width/env_height are given, otherwise in
    heatmap-cell-index units.

    When use_color is True, lines are coloured by strength of alignment using
    the polybee colormap instead of a grayscale shade.

    Cells are only drawn if their strength is >= strength_th and their count
    fraction (count / max_count) is >= count_th.
    """
    cmap = create_polybee_colormap() if use_color else None
    if env_width is not None and env_height is not None:
        fm_cell_w = env_width / fm_cols
        fm_cell_h = env_height / fm_rows
    elif heatmap_nrows is not None and heatmap_ncols is not None:
        fm_cell_w = heatmap_ncols / fm_cols
        fm_cell_h = heatmap_nrows / fm_rows
    else:
        fm_cell_w = 1.0
        fm_cell_h = 1.0

    max_count = max((cell[2] for row in cells for cell in row), default=0)
    if max_count == 0:
        return

    segments = []
    colors = []
    linewidths = []

    for r in range(fm_rows):
        for c in range(fm_cols):
            axis, strength, count = cells[r][c]
            if strength <= 0.0 or count == 0:
                continue

            count_fraction = count / max_count
            if strength < strength_th or count_fraction < count_th:
                continue

            cx = (c + 0.5) * fm_cell_w
            cy = (r + 0.5) * fm_cell_h

            half_len = min(fm_cell_w, fm_cell_h) * 0.45
            dx = half_len * math.cos(axis)
            dy = half_len * math.sin(axis)

            segments.append([(cx - dx, cy - dy), (cx + dx, cy + dy)])

            if use_color:
                colors.append(cmap(strength))
            else:
                shade = (75.0 / 255.0) * (1.0 - strength)
                colors.append((shade, shade, shade, 1.0))

            linewidths.append(1.0 + count_fraction * 4.0)

    if not segments:
        return

    lc = LineCollection(segments, colors=colors, linewidths=linewidths,
                        capstyle='round', zorder=5)
    ax.add_collection(lc)


def load_heatmap(filename):
    """Load heatmap data from CSV file."""
    try:
        data = np.loadtxt(filename, delimiter=',')
        return data
    except Exception as e:
        print(f"Error loading CSV file: {e}", file=sys.stderr)
        sys.exit(1)


def visualize_heatmap(data, title="Heatmap", save_only=False, output_file=None,
                      env_width=None, env_height=None,
                      tunnel=None, entrances=None, crop_patches=None, hive=None,
                      flowmap=None, color_scale_max=None):
    nrows, ncols = data.shape
    scale = min(10.0 / max(nrows, ncols), 0.7)  # inches per cell, capped so figure stays reasonable
    top_margin = 1.0    # title
    bottom_margin = 1.5  # x-axis tick labels + axis label
    fig_w = ncols * scale + 2.5
    fig_h = min(nrows * scale + top_margin + bottom_margin, 9.0)  # cap height for small screens
    _, ax = plt.subplots(figsize=(fig_w, fig_h))

    # Create custom colormap matching LocalVis.cpp
    cmap = create_polybee_colormap()

    # When env dimensions are known, use extent to label axes in env units.
    # extent=[left, right, bottom, top] with top=0 so row 0 is at the top.
    if env_width is not None and env_height is not None:
        extent = [0, env_width, env_height, 0]
        cell_w = env_width / ncols
        cell_h = env_height / nrows
        xlabel = 'X (env units)'
        ylabel = 'Y (env units)'
    else:
        extent = None
        cell_w = 1.0
        cell_h = 1.0
        xlabel = 'X coordinate'
        ylabel = 'Y coordinate'

    # aspect='equal' keeps cells square; extent sets the axis coordinate range
    if color_scale_max is not None:
        im = ax.imshow(data, cmap=cmap, interpolation='nearest', aspect='equal', extent=extent,
                       vmin=0.0, vmax=color_scale_max)
    else:
        im = ax.imshow(data, cmap=cmap, interpolation='nearest', aspect='equal', extent=extent)

    if env_width is not None and env_height is not None:
        ax.set_xticks(np.arange(0, env_width + 1, 50))
        ax.set_yticks(np.arange(0, env_height + 1, 50))

        # Draw environment overlays
        if crop_patches:
            _draw_crop_rows(ax, crop_patches)
        if tunnel:
            _draw_tunnel(ax, tunnel, entrances or [])
        if hive is not None:
            _draw_hive(ax, hive)

    if flowmap is not None:
        cells, fm_rows, fm_cols = flowmap
        _draw_flowmap(ax, cells, fm_rows, fm_cols,
                      env_width=env_width, env_height=env_height,
                      heatmap_nrows=nrows, heatmap_ncols=ncols)

    # Add colorbar
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Value' if color_scale_max is not None else 'Normalized Value', rotation=270, labelpad=20)

    # Set title and labels
    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)

    # Add grid for better readability
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)

    # Add text annotations for each cell (optional, only for small heatmaps)
    if nrows <= 20 and ncols <= 20:  # Only annotate if heatmap is small enough
        for i in range(nrows):
            for j in range(ncols):
                ax.text((j + 0.5) * cell_w, (i + 0.5) * cell_h, f'{data[i, j]:.2f}',
                        ha="center", va="center", color="black", fontsize=8)

    plt.tight_layout(pad=1.2)

    if save_only:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Heatmap saved to: {output_file}")
        plt.close()
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Visualize a heatmap from a CSV file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s heatmap.csv                          # Display heatmap interactively
  %(prog)s heatmap.csv --save-only              # Save to heatmap.png without displaying
  %(prog)s heatmap.csv -c polybee.cfg -f flowmap.csv   # Overlay flowmap
  %(prog)s angdelta-heatmap.csv --color-scale-max 1.5708   # Fixed 0-N colour scale instead of auto-scaling
        """
    )

    parser.add_argument('input_file',
                       help='Input CSV file containing a 2D grid of heatmap values')
    parser.add_argument('--save-only',
                       action='store_true',
                       help='Save to PNG file without displaying on screen')
    parser.add_argument('--config', '-c',
                       metavar='CFG',
                       help='Polybee config file; if supplied, axes are labelled in env units '
                            'and tunnel, crop rows, and hive are drawn as overlays')
    parser.add_argument('--flowmap', '-f',
                       metavar='FLOWMAP_CSV',
                       help='Flowmap CSV file (from Flowmap::print) to superimpose over the heatmap')
    parser.add_argument('--title', '-t',
                       metavar='TITLE',
                       help='Title for the plot (default: input filename without extension)')
    parser.add_argument('--color-scale-max',
                       type=float, metavar='N',
                       help='Fix the colour scale to run from 0 to N instead of auto-scaling to '
                            'the data range.')

    args = parser.parse_args()

    # Check if input file exists
    if not os.path.isfile(args.input_file):
        print(f"Error: File not found: {args.input_file}", file=sys.stderr)
        sys.exit(1)

    if args.color_scale_max is not None and args.color_scale_max <= 0.0:
        print(f"Error: --color-scale-max must be a positive number, got {args.color_scale_max}", file=sys.stderr)
        sys.exit(1)

    # Load the heatmap data
    data = load_heatmap(args.input_file)

    env_width, env_height = None, None
    tunnel, entrances, crop_patches, hive = {}, [], [], None
    if args.config:
        env_width, env_height, tunnel, entrances, crop_patches, hive = parse_config(args.config)

    flowmap = None
    if args.flowmap:
        if not os.path.isfile(args.flowmap):
            print(f"Error: Flowmap file not found: {args.flowmap}", file=sys.stderr)
            sys.exit(1)
        flowmap = load_flowmap(args.flowmap)

    # Get the basename without extension for the default title and output file
    basename = os.path.splitext(os.path.basename(args.input_file))[0]
    title = args.title if args.title else basename

    if args.save_only:
        # Generate output filename: replace .csv with .png
        output_file = os.path.splitext(args.input_file)[0] + '.png'
        visualize_heatmap(data, title=title, save_only=True, output_file=output_file,
                          env_width=env_width, env_height=env_height,
                          tunnel=tunnel, entrances=entrances,
                          crop_patches=crop_patches, hive=hive,
                          flowmap=flowmap, color_scale_max=args.color_scale_max)
    else:
        visualize_heatmap(data, title=title, save_only=False,
                          env_width=env_width, env_height=env_height,
                          tunnel=tunnel, entrances=entrances,
                          crop_patches=crop_patches, hive=hive,
                          flowmap=flowmap, color_scale_max=args.color_scale_max)


if __name__ == '__main__':
    main()
