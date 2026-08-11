#!/usr/bin/env python3
"""
Visualize the physical environment described by a polybee config file: the
tunnel outline (with entrance gaps), crop rows, hive location -- the same
overlay elements drawn by visualize_heatmap.py/visualize_flowmap.py when
given -c/--config -- plus any barriers and bridges specified in the file,
drawn with their actual position, extent, and (for barriers) orientation.

Barriers are drawn as line segments (any `nrx,dx`/`nry,dy` repeat grid is
expanded into individual instances, as Environment::initialiseBarriers does).
Bridges are drawn as filled squares; a `patch` entry is identified as a
bridge, rather than an ordinary crop-row patch, when its species id is 0 and
it is not repeated (the same convention used by gen_bx_heatmaps.py) --
ordinary crop-row patches are drawn via the same _draw_crop_rows() used by
visualize_heatmap.py.

This is typically run over the `best-configs-indiv/*.cfg` files written by
run_analysis.sh, to see the exact barrier/bridge layout of the champion
individual from each replicate evolutionary run, alongside the aggregate
barrier/bridge density maps from gen_bx_heatmaps.py / gen_barrier_flowmap.py.

By default, unlike visualize_heatmap.py/visualize_flowmap.py, the image is
saved directly (no display window) since this is intended to be run
unattended over many config files; pass --show to also display it.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./visualize_config.py CONFIG
    ./visualize_config.py CONFIG --show
    ./visualize_config.py CONFIG --title "Champion individual, run 10"
    ./visualize_config.py CONFIG --cell-size 50

Example:
    ./visualize_config.py best-configs-indiv/best-evolve-20X-10B-...-10.cfg
    # -> writes best-configs-indiv/best-evolve-20X-10B-...-10.png
"""

import argparse
import math
import os
import sys

import matplotlib.lines as mlines
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection

from visualize_heatmap import parse_config, _draw_tunnel, _draw_crop_rows, _draw_hive

BARRIER_COLOR = '#d62728'  # distinct from the tunnel wall's black
BRIDGE_COLOR = '#9467bd'   # distinct from crop rows' light grey


def _parse_barrier_segments(value):
    """Parse a `barrier` value (x1,y1:x2,y2[:nrx,dx[:nry,dy]]) into a list of
    (x1, y1, x2, y2) endpoint tuples, one per repeat instance, expanding any
    repeat grid exactly as Environment::initialiseBarriers does."""
    parts = value.split(':')
    if len(parts) < 2:
        return []
    p1 = parts[0].split(',')
    p2 = parts[1].split(',')
    if len(p1) < 2 or len(p2) < 2:
        return []
    try:
        x1, y1 = float(p1[0]), float(p1[1])
        x2, y2 = float(p2[0]), float(p2[1])
    except ValueError:
        return []

    num_repeats_x, dx = 1, 0.0
    num_repeats_y, dy = 1, 0.0
    if len(parts) >= 3:
        p3 = parts[2].split(',')
        if len(p3) >= 2:
            try:
                num_repeats_x, dx = int(p3[0]), float(p3[1])
            except ValueError:
                num_repeats_x, dx = 1, 0.0
    if len(parts) >= 4:
        p4 = parts[3].split(',')
        if len(p4) >= 2:
            try:
                num_repeats_y, dy = int(p4[0]), float(p4[1])
            except ValueError:
                num_repeats_y, dy = 1, 0.0

    segments = []
    for i in range(num_repeats_x):
        for j in range(num_repeats_y):
            segments.append((x1 + i * dx, y1 + j * dy, x2 + i * dx, y2 + j * dy))
    return segments


def _parse_bridge_rect(value):
    """Return (x, y, w, h) for a `patch` value if it represents a bridge
    patch (species id 0, not repeated), else None. Matches the bridge
    identification convention used in gen_bx_heatmaps.py."""
    parts = value.split(':')
    if len(parts) < 5:
        return None
    if parts[3].strip() != '0':
        return None
    try:
        repeat = float(parts[4].strip())
    except ValueError:
        return None
    if repeat > 1:
        return None
    geom = parts[0].split(',')
    if len(geom) < 4:
        return None
    try:
        x, y, w, h = (float(geom[0]), float(geom[1]),
                      float(geom[2]), float(geom[3]))
    except ValueError:
        return None
    return (x, y, w, h)


def parse_barriers_and_bridges(config_file):
    """Return (barrier_segments, bridges): a list of (x1,y1,x2,y2) tuples and
    a list of (x,y,w,h) tuples, read from a polybee config file's `barrier`
    and `patch` entries."""
    barrier_segments = []
    bridges = []
    try:
        with open(config_file) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#') or '=' not in line:
                    continue
                key, _, value = line.partition('=')
                key = key.strip()
                value = value.strip()

                if key == 'barrier':
                    barrier_segments.extend(_parse_barrier_segments(value))
                elif key == 'patch':
                    bridge = _parse_bridge_rect(value)
                    if bridge is not None:
                        bridges.append(bridge)
    except OSError as e:
        print(f"Error reading config file: {e}", file=sys.stderr)
        sys.exit(1)

    return barrier_segments, bridges


def _draw_barriers(ax, barrier_segments):
    if not barrier_segments:
        return
    lc = LineCollection(
        [((x1, y1), (x2, y2)) for x1, y1, x2, y2 in barrier_segments],
        colors=BARRIER_COLOR, linewidths=3.0, capstyle='round', zorder=6,
    )
    ax.add_collection(lc)


def _draw_bridges(ax, bridges):
    for x, y, w, h in bridges:
        ax.add_patch(mpatches.Rectangle(
            (x, y), w, h,
            linewidth=1.5, edgecolor=BRIDGE_COLOR, facecolor=BRIDGE_COLOR,
            alpha=0.5, zorder=5,
        ))


def visualize_config(env_width, env_height, tunnel, entrances, crop_patches, hive,
                     barrier_segments, bridges, title="Environment configuration",
                     output_file=None, show=False, cell_size=25):
    # Figure sizing uses the same formula as visualize_heatmap.py/
    # visualize_flowmap.py, with nrows/ncols derived from env dimensions and
    # --cell-size, so images line up in size with a same-cell-size heatmap
    # or flowmap of the same environment.
    ncols = max(1, math.ceil(env_width / cell_size))
    nrows = max(1, math.ceil(env_height / cell_size))
    scale = min(10.0 / max(nrows, ncols), 0.7)  # inches per cell, capped so figure stays reasonable
    top_margin = 1.0     # title
    bottom_margin = 1.5  # x-axis tick labels + axis label
    fig_w = ncols * scale + 2.5
    fig_h = min(nrows * scale + top_margin + bottom_margin, 9.0)  # cap height for small screens
    _, ax = plt.subplots(figsize=(fig_w, fig_h))

    # Axis coordinates run top-to-bottom in increasing y, same convention as
    # the imshow(extent=...) used in visualize_heatmap.py.
    ax.set_xlim(0, env_width)
    ax.set_ylim(env_height, 0)
    ax.set_xticks(np.arange(0, env_width + 1, 50))
    ax.set_yticks(np.arange(0, env_height + 1, 50))
    ax.set_aspect('equal')
    ax.set_facecolor('white')

    if crop_patches:
        _draw_crop_rows(ax, crop_patches)
    if tunnel:
        _draw_tunnel(ax, tunnel, entrances or [])
    if hive is not None:
        _draw_hive(ax, hive)
    _draw_barriers(ax, barrier_segments)
    _draw_bridges(ax, bridges)

    legend_handles = [
        mlines.Line2D([], [], color='black', lw=2, label='Tunnel wall'),
        mpatches.Patch(edgecolor='#a8a8a8', facecolor='none', lw=2, label='Crop row'),
        mpatches.Patch(edgecolor='#e8a000', facecolor='none', lw=2, label='Hive'),
    ]
    if barrier_segments:
        legend_handles.append(mlines.Line2D([], [], color=BARRIER_COLOR, lw=3, label='Barrier'))
    if bridges:
        legend_handles.append(mpatches.Patch(edgecolor=BRIDGE_COLOR, facecolor=BRIDGE_COLOR,
                                             alpha=0.5, label='Bridge'))
    ax.legend(handles=legend_handles, loc='upper right', fontsize=8, framealpha=0.9)

    ax.set_title(title, fontsize=14, fontweight='bold', pad=20)
    ax.set_xlabel('X (env units)', fontsize=12)
    ax.set_ylabel('Y (env units)', fontsize=12)
    ax.grid(True, alpha=0.3, linestyle='--', linewidth=0.5)

    plt.tight_layout(pad=1.2)

    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Config map saved to: {output_file}")

    if show:
        plt.show()
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Visualize the environment (tunnel, entrances, crop rows, hive, '
                    'barriers, bridges) described by a polybee config file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s best-config.cfg                       # Save best-config.png
  %(prog)s best-config.cfg --show                # Save, then also display
  %(prog)s best-config.cfg -t "Champion, run 10"  # Custom title
  %(prog)s best-config.cfg --cell-size 50         # Match a cell-size-50 heatmap's dimensions
        """
    )

    parser.add_argument('config_file', help='Polybee config file to visualize')
    parser.add_argument('--title', '-t',
                        metavar='TITLE',
                        help='Title for the plot (default: input filename without extension)')
    parser.add_argument('--cell-size',
                        type=int, default=25, metavar='N',
                        help='Cell size in environment units, used only to size the output '
                             'image to match a same-cell-size heatmap/flowmap (default: 25)')
    parser.add_argument('--show',
                        action='store_true',
                        help='Also display the image interactively after saving it')

    args = parser.parse_args()

    if not os.path.isfile(args.config_file):
        print(f"Error: File not found: {args.config_file}", file=sys.stderr)
        sys.exit(1)

    if args.cell_size <= 0:
        print(f"Error: --cell-size must be positive, got {args.cell_size}", file=sys.stderr)
        sys.exit(1)

    env_width, env_height, tunnel, entrances, crop_patches, hive = parse_config(args.config_file)
    barrier_segments, bridges = parse_barriers_and_bridges(args.config_file)

    basename = os.path.splitext(os.path.basename(args.config_file))[0]
    title = args.title if args.title else basename
    output_file = os.path.splitext(args.config_file)[0] + '.png'

    visualize_config(env_width, env_height, tunnel, entrances, crop_patches, hive,
                     barrier_segments, bridges, title=title,
                     output_file=output_file, show=args.show, cell_size=args.cell_size)


if __name__ == '__main__':
    main()
