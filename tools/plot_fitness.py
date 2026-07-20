#!/usr/bin/env python3
"""
Plot fitness scores from archipelago optimization run with multiple islands.

Interactive UI allows toggling display of:
- Individual island results
- Overall archipelago results
- Different metrics (mean, median, min, individual scores)

Input CSV format: island_number,generation_number,fitness_score

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./plot_fitness.py [-t {0,1}] <input_csv_file>
    ./plot_fitness.py <input_csv_file> --ymin 0 --ymax 2
    ./plot_fitness.py <input_csv_file> --minimal
    ./plot_fitness.py <input_csv_file> --save-only --basename myexpt

    # Multiple input files: instead of per-island/per-file lines, plots the
    # median (across files) of each file's per-generation median and min,
    # with the interquartile range shaded around each line.
    ./plot_fitness.py fitness-expt_1.csv fitness-expt_2.csv fitness-expt_3.csv
    ./plot_fitness.py fitness-expt_*.csv --minimal
"""

import matplotlib.pyplot as plt
from matplotlib.widgets import CheckButtons, Button
import numpy as np
from collections import defaultdict
import os
import re
import sys
import argparse


OBJECTIVE_LABELS = {
    0: {'name': 'EMD Score', 'short': 'EMD'},
    1: {'name': 'Visit Success Fraction', 'short': 'Visit Success'},
}


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='Plot fitness scores from archipelago optimization run')
    parser.add_argument('input_files', nargs='+',
                        help='Input CSV file(s) (island,generation,fitness_score). If more '
                             'than one file is given, plots the median (across files) of each '
                             "file's per-generation median and min, with the interquartile "
                             'range shaded around each line, instead of per-file lines.')
    parser.add_argument('-t', '--type', type=int, default=0, choices=[0, 1],
                        help='Objective function type: 0=EMD distance to target (default), '
                             '1=Flower visit count success fraction')
    parser.add_argument('--show-means', action='store_true', default=False,
                        help='Include mean data and labels in plot (off by default)')
    parser.add_argument('--title', type=str, default=None,
                        help='Optional title to display above the plot')
    parser.add_argument('--ymin', type=float, default=None,
                        help='Minimum value for the y-axis scale (default: auto-scaled)')
    parser.add_argument('--ymax', type=float, default=None,
                        help='Maximum value for the y-axis scale (default: auto-scaled)')
    parser.add_argument('--minimal', action='store_true', default=False,
                        help='Show only the graph, axis labels, and legend, without the '
                             'island/metric controls or stats panel on the right')
    parser.add_argument('--one-island', action='store_true', default=False,
                        help='Indicate the run only consisted of a single island: omit the '
                             '"Archipelago (1 Islands) -" prefix from the plot title, and drop '
                             '"Archipelago" from the Median/Mean/Min legend labels')
    parser.add_argument('--save-only', action='store_true', default=False,
                        help='Save the plot to graph-fitness-BASENAME-N.png instead of '
                             'displaying it on screen, where N is the run number taken from '
                             'the input filename')
    parser.add_argument('--basename', type=str, default='expt',
                        help='Basename used in the --save-only output filename (default: expt)')
    return parser.parse_args()


RUN_NUM_RE = re.compile(r'[-_](\d+)\.[^.]+$')


def extract_run_number(filepath):
    """Return the trailing run number N from a filename like fitness-expt-N.csv, or None."""
    match = RUN_NUM_RE.search(os.path.basename(filepath))
    if not match:
        return None
    return int(match.group(1))


def read_data(filename):
    """Read CSV data and return island_data dict and list of all islands."""
    island_data = defaultdict(lambda: defaultdict(list))
    all_islands = set()

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line:
                parts = line.split(',')
                if len(parts) == 3:
                    try:
                        island = int(parts[0])
                        gen = int(parts[1])
                        score = float(parts[2])
                        island_data[island][gen].append(score)
                        all_islands.add(island)
                    except ValueError:
                        continue

    return island_data, sorted(all_islands)


def calculate_generation_stats(data_dict):
    """Calculate mean, median, and min scores per generation from a data dictionary."""
    unique_gens = sorted(data_dict.keys())
    mean_scores = [np.mean(data_dict[g]) for g in unique_gens]
    median_scores = [np.median(data_dict[g]) for g in unique_gens]
    min_scores = [np.min(data_dict[g]) for g in unique_gens]

    # Collect individual scores with their generations
    all_gens = []
    all_scores = []
    for gen in unique_gens:
        all_gens.extend([gen] * len(data_dict[gen]))
        all_scores.extend(data_dict[gen])

    return unique_gens, mean_scores, median_scores, min_scores, all_gens, all_scores


def calculate_rolling_average(scores, window_size=10):
    """Calculate rolling average if enough data points exist."""
    if len(scores) >= window_size:
        return np.convolve(scores, np.ones(window_size)/window_size, mode='valid')
    return None


class InteractivePlot:
    """Interactive plot with toggle controls for islands and metrics."""

    def __init__(self, island_data, all_islands, objective_type=0, show_means=False, title=None,
                ymin=None, ymax=None, minimal=False, one_island=False):
        self.island_data = island_data
        self.all_islands = all_islands
        self.score_name = OBJECTIVE_LABELS[objective_type]['name']
        self.score_short = OBJECTIVE_LABELS[objective_type]['short']
        self.show_means = show_means
        self.title = title
        self.ymin = ymin
        self.ymax = ymax
        self.minimal = minimal
        self.one_island = one_island
        self.num_islands = len(all_islands)

        # Combine all islands' data for archipelago-level statistics
        self.archipelago_scores = defaultdict(list)
        for island in all_islands:
            for gen in island_data[island]:
                self.archipelago_scores[gen].extend(island_data[island][gen])

        # Generate distinct colors for each island. tab10 only has 10 discrete
        # colors, so sampling it at >10 points (via np.linspace) collides on
        # duplicates; fall back to a continuously-interpolated colormap once
        # there are more than 10 islands.
        if self.num_islands <= 10:
            self.colors = plt.cm.tab10(np.linspace(0, 1, 10))
        else:
            self.colors = plt.cm.hsv(np.linspace(0, 1, self.num_islands, endpoint=False))

        # Store plot elements for toggling
        self.island_plots = {}  # island -> {'mean': line, 'median': line, 'min': line, 'individual': scatter, 'rolling': line}
        self.archipelago_plots = {}  # metric -> plot element

        # Visibility state
        self.island_visible = {island: True for island in all_islands}
        self.island_visible['archipelago'] = True
        self.metric_visible = {
            'mean': show_means,
            'median': True,
            'min': True,
            'individual': False,  # Off by default as it can be cluttered
            'rolling': True
        }

        self.setup_figure()
        self.create_plots()
        if self.ymin is not None or self.ymax is not None:
            self.ax.set_ylim(bottom=self.ymin, top=self.ymax)
        if not self.minimal:
            self.create_controls()
            self.update_stats_text()

    def setup_figure(self):
        """Set up the figure and axes layout."""
        self.fig = plt.figure(figsize=(10, 8) if self.minimal else (16, 9))

        if self.minimal:
            # No controls/stats panel, so the plot can use most of the figure
            self.ax = self.fig.add_axes([0.10, 0.1, 0.85, 0.8])
        else:
            # Main plot area (leave room for controls on the right)
            self.ax = self.fig.add_axes([0.08, 0.1, 0.55, 0.8])

        self.ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
        self.ax.set_ylabel(self.score_name, fontsize=12, fontweight='bold')
        # score_name is deliberately not repeated here -- it's already shown
        # as the y-axis label.
        base_title = '' if self.one_island else f'Archipelago ({self.num_islands} Islands)'
        full_title = '\n'.join(part for part in (self.title, base_title) if part)
        self.ax.set_title(full_title, fontsize=14, fontweight='bold')
        self.ax.grid(True, alpha=0.3, linestyle='--')

    def create_plots(self):
        """Create all plot elements (initially visible based on defaults)."""
        # Plot each island (skipped in --minimal mode, which shows only the
        # overall archipelago data)
        if not self.minimal:
            for idx, island in enumerate(self.all_islands):
                color = self.colors[idx % len(self.colors)]
                self.island_plots[island] = self.create_island_plots(island, color)

        # Plot archipelago results
        self.archipelago_plots = self.create_archipelago_plots()

        self.ax.legend(loc='upper right', framealpha=0.9, fontsize=8)

    def create_island_plots(self, island, color):
        """Create plot elements for a single island."""
        gen_scores = self.island_data[island]
        unique_gens, mean_scores, median_scores, min_scores, all_gens, all_scores = \
            calculate_generation_stats(gen_scores)

        plots = {}

        # Individual scores (scatter)
        scatter = self.ax.scatter(all_gens, all_scores, alpha=0.2, s=15, c=[color],
                                  edgecolors='none', visible=self.metric_visible['individual'])
        plots['individual'] = scatter

        # Mean line (only if --show-means)
        if self.show_means:
            mean_line, = self.ax.plot(unique_gens, mean_scores, color=color, linewidth=1.0,
                                      alpha=0.4, linestyle='--', marker='o', markersize=2,
                                      visible=self.metric_visible['mean'])
            plots['mean'] = mean_line
        else:
            plots['mean'] = None

        # Median line
        median_line, = self.ax.plot(unique_gens, median_scores, color=color, linewidth=1.5,
                                    alpha=0.6, label=f'Island {island}', marker='^', markersize=3,
                                    visible=self.metric_visible['median'])
        plots['median'] = median_line

        # Min line
        min_line, = self.ax.plot(unique_gens, min_scores, color=color, linewidth=1.0,
                                 alpha=0.4, linestyle=':', marker='s', markersize=2,
                                 visible=self.metric_visible['min'])
        plots['min'] = min_line

        # Rolling average of median
        window_size = 10
        median_rolling = calculate_rolling_average(median_scores, window_size)
        if median_rolling is not None:
            rolling_gens = unique_gens[window_size-1:]
            rolling_line, = self.ax.plot(rolling_gens, median_rolling, color=color,
                                         linewidth=2.0, linestyle='-', alpha=0.8,
                                         visible=self.metric_visible['rolling'])
            plots['rolling'] = rolling_line
        else:
            plots['rolling'] = None

        return plots

    def create_archipelago_plots(self):
        """Create plot elements for the archipelago (combined) results."""
        unique_gens, mean_scores, median_scores, min_scores, all_gens, all_scores = \
            calculate_generation_stats(self.archipelago_scores)

        plots = {}

        # With --one-island, the "Archipelago" qualifier is redundant since
        # there's only a single island contributing to these stats.
        arch_prefix = '' if self.one_island else 'Archipelago '

        # Individual scores (scatter)
        scatter = self.ax.scatter(all_gens, all_scores, alpha=0.15, s=20, c='gray',
                                  edgecolors='none', visible=self.metric_visible['individual'])
        plots['individual'] = scatter

        # Mean line (only if --show-means)
        if self.show_means:
            mean_line, = self.ax.plot(unique_gens, mean_scores, color='black', linewidth=1.5,
                                      alpha=0.5, linestyle=':', marker='o', markersize=3,
                                      label=f'{arch_prefix}Mean', visible=self.metric_visible['mean'])
            plots['mean'] = mean_line
        else:
            plots['mean'] = None

        # Median line
        median_line, = self.ax.plot(unique_gens, median_scores, color='orange', linewidth=1.5,
                                    alpha=0.5, linestyle=':', marker='^', markersize=3,
                                    label=f'{arch_prefix}Median', visible=self.metric_visible['median'])
        plots['median'] = median_line

        # Min line
        min_line, = self.ax.plot(unique_gens, min_scores, color='mediumseagreen', linewidth=1.5,
                                 alpha=0.5, linestyle='-', marker='s', markersize=3,
                                 label=f'{arch_prefix}Min', visible=self.metric_visible['min'])
        plots['min'] = min_line

        # Rolling averages
        window_size = 10
        mean_rolling = calculate_rolling_average(mean_scores, window_size)
        median_rolling = calculate_rolling_average(median_scores, window_size)
        min_rolling = calculate_rolling_average(min_scores, window_size)

        if median_rolling is not None:
            rolling_gens = unique_gens[window_size-1:]

            if self.show_means and mean_rolling is not None:
                mean_roll_line, = self.ax.plot(rolling_gens, mean_rolling, color='darkblue',
                                               linewidth=2.5, linestyle='-',
                                               label=f'{arch_prefix}Mean (rolling)',
                                               visible=self.metric_visible['rolling'] and self.metric_visible['mean'])
                plots['mean_rolling'] = mean_roll_line
            else:
                plots['mean_rolling'] = None

            median_roll_line, = self.ax.plot(rolling_gens, median_rolling, color='chocolate',
                                             linewidth=2.5, linestyle='-',
                                             label=f'{arch_prefix}Median (rolling)',
                                             visible=self.metric_visible['rolling'] and self.metric_visible['median'])
            plots['median_rolling'] = median_roll_line

            min_roll_line, = self.ax.plot(rolling_gens, min_rolling, color='darkgreen',
                                          linewidth=2.5, linestyle='-',
                                          label=f'{arch_prefix}Min (rolling)',
                                          visible=self.metric_visible['rolling'] and self.metric_visible['min'])
            plots['min_rolling'] = min_roll_line
        else:
            plots['mean_rolling'] = None
            plots['median_rolling'] = None
            plots['min_rolling'] = None

        # Store stats for text display
        self.arch_stats = {
            'unique_gens': unique_gens,
            'mean_scores': mean_scores,
            'median_scores': median_scores,
            'min_scores': min_scores,
            'mean_rolling': mean_rolling,
            'median_rolling': median_rolling,
            'min_rolling': min_rolling
        }

        return plots

    def create_controls(self):
        """Create checkbox controls for toggling visibility."""
        # Island checkboxes
        island_labels = [f'Island {i}' for i in self.all_islands] + ['Archipelago']
        island_states = [self.island_visible[i] for i in self.all_islands] + [self.island_visible['archipelago']]

        # Position for island checkboxes
        ax_islands = self.fig.add_axes([0.68, 0.35, 0.12, 0.55])
        ax_islands.set_title('Data Sources', fontsize=10, fontweight='bold')
        self.check_islands = CheckButtons(ax_islands, island_labels, island_states)
        self.check_islands.on_clicked(self.toggle_island)

        # Select All / Deselect All buttons for islands
        ax_select_all = self.fig.add_axes([0.68, 0.28, 0.055, 0.04])
        self.btn_select_all = Button(ax_select_all, 'All', hovercolor='lightgreen')
        self.btn_select_all.on_clicked(self.select_all_islands)

        ax_deselect_all = self.fig.add_axes([0.745, 0.28, 0.055, 0.04])
        self.btn_deselect_all = Button(ax_deselect_all, 'None', hovercolor='lightcoral')
        self.btn_deselect_all.on_clicked(self.deselect_all_islands)

        # Metric checkboxes
        metric_labels = []
        metric_keys = []
        if self.show_means:
            metric_labels.append(f'Mean {self.score_short}')
            metric_keys.append('mean')
        metric_labels += [f'Median {self.score_short}', f'Min {self.score_short}',
                          'Individual Scores', 'Rolling Avg']
        metric_keys += ['median', 'min', 'individual', 'rolling']
        metric_states = [self.metric_visible[k] for k in metric_keys]
        self.metric_keys = metric_keys

        ax_metrics = self.fig.add_axes([0.82, 0.5, 0.15, 0.4])
        ax_metrics.set_title('Metrics', fontsize=10, fontweight='bold')
        self.check_metrics = CheckButtons(ax_metrics, metric_labels, metric_states)
        self.check_metrics.on_clicked(self.toggle_metric)

        # Select All / Deselect All buttons for metrics
        ax_metric_all = self.fig.add_axes([0.82, 0.43, 0.07, 0.04])
        self.btn_metric_all = Button(ax_metric_all, 'All', hovercolor='lightgreen')
        self.btn_metric_all.on_clicked(self.select_all_metrics)

        ax_metric_none = self.fig.add_axes([0.90, 0.43, 0.07, 0.04])
        self.btn_metric_none = Button(ax_metric_none, 'None', hovercolor='lightcoral')
        self.btn_metric_none.on_clicked(self.deselect_all_metrics)

        # Stats text area
        self.stats_text_ax = self.fig.add_axes([0.68, 0.05, 0.30, 0.20])
        self.stats_text_ax.axis('off')

    def toggle_island(self, label):
        """Toggle visibility of an island's plots."""
        if label == 'Archipelago':
            self.island_visible['archipelago'] = not self.island_visible['archipelago']
            self.update_archipelago_visibility()
        else:
            island = int(label.split()[-1])
            self.island_visible[island] = not self.island_visible[island]
            self.update_island_visibility(island)

        self.update_legend()
        self.fig.canvas.draw_idle()

    def toggle_metric(self, label):
        """Toggle visibility of a metric type."""
        label_to_key = {
            f'Mean {self.score_short}': 'mean',
            f'Median {self.score_short}': 'median',
            f'Min {self.score_short}': 'min',
            'Individual Scores': 'individual',
            'Rolling Avg': 'rolling'
        }
        key = label_to_key[label]
        self.metric_visible[key] = not self.metric_visible[key]

        # Update all plots
        for island in self.all_islands:
            self.update_island_visibility(island)
        self.update_archipelago_visibility()

        self.update_legend()
        self.fig.canvas.draw_idle()

    def update_island_visibility(self, island):
        """Update visibility of all plot elements for an island."""
        plots = self.island_plots[island]
        island_vis = self.island_visible[island]

        for metric in ['mean', 'median', 'min', 'individual']:
            if plots[metric] is not None:
                plots[metric].set_visible(island_vis and self.metric_visible[metric])

        if plots['rolling'] is not None:
            plots['rolling'].set_visible(island_vis and self.metric_visible['rolling'] and self.metric_visible['median'])

    def update_archipelago_visibility(self):
        """Update visibility of all archipelago plot elements."""
        plots = self.archipelago_plots
        arch_vis = self.island_visible['archipelago']

        for metric in ['mean', 'median', 'min', 'individual']:
            if plots[metric] is not None:
                plots[metric].set_visible(arch_vis and self.metric_visible[metric])

        # Rolling averages
        if plots['mean_rolling'] is not None:
            plots['mean_rolling'].set_visible(arch_vis and self.metric_visible['rolling'] and self.metric_visible['mean'])
        if plots['median_rolling'] is not None:
            plots['median_rolling'].set_visible(arch_vis and self.metric_visible['rolling'] and self.metric_visible['median'])
        if plots['min_rolling'] is not None:
            plots['min_rolling'].set_visible(arch_vis and self.metric_visible['rolling'] and self.metric_visible['min'])

    def select_all_islands(self, event):
        """Select all islands and archipelago."""
        # set_active() triggers the toggle_island callback which handles state updates
        # So we only call set_active() when item is currently not visible (unchecked)
        for i, island in enumerate(self.all_islands):
            if not self.island_visible[island]:
                self.check_islands.set_active(i)
        if not self.island_visible['archipelago']:
            self.check_islands.set_active(len(self.all_islands))

    def deselect_all_islands(self, event):
        """Deselect all islands and archipelago."""
        # set_active() triggers the toggle_island callback which handles state updates
        # So we only call set_active() when item is currently visible (checked)
        for i, island in enumerate(self.all_islands):
            if self.island_visible[island]:
                self.check_islands.set_active(i)
        if self.island_visible['archipelago']:
            self.check_islands.set_active(len(self.all_islands))

    def select_all_metrics(self, event):
        """Select all metrics."""
        # set_active() triggers the toggle_metric callback which handles state updates
        for i, key in enumerate(self.metric_keys):
            if not self.metric_visible[key]:
                self.check_metrics.set_active(i)

    def deselect_all_metrics(self, event):
        """Deselect all metrics."""
        # set_active() triggers the toggle_metric callback which handles state updates
        for i, key in enumerate(self.metric_keys):
            if self.metric_visible[key]:
                self.check_metrics.set_active(i)

    def update_legend(self):
        """Update the legend to only show visible items."""
        self.ax.legend(loc='upper right', framealpha=0.9, fontsize=8)

    def update_stats_text(self):
        """Update the statistics text display."""
        stats = self.arch_stats
        unique_gens = stats['unique_gens']
        mean_scores = stats['mean_scores']
        median_scores = stats['median_scores']
        min_scores = stats['min_scores']

        final_gen = unique_gens[-1]
        final_mean = mean_scores[-1]
        final_median = median_scores[-1]
        final_min = min_scores[-1]

        all_scores = [score for gen_scores in self.archipelago_scores.values()
                      for score in gen_scores]
        overall_min = np.min(all_scores)

        stats_text = f'Archipelago Statistics\n'
        stats_text += f'─' * 25 + '\n'
        stats_text += f'Final Generation: {final_gen}\n'
        if self.show_means:
            stats_text += f'Final Mean:   {final_mean:.4f}\n'
        stats_text += f'Final Median: {final_median:.4f}\n'
        stats_text += f'Final Min:    {final_min:.4f}\n'
        stats_text += f'Overall Min:  {overall_min:.4f}'

        self.stats_text_ax.clear()
        self.stats_text_ax.axis('off')
        self.stats_text_ax.text(0.0, 1.0, stats_text,
                                transform=self.stats_text_ax.transAxes,
                                verticalalignment='top',
                                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
                                fontsize=9, family='monospace')

    def show(self):
        """Display the plot."""
        plt.show()

    def save(self, output_file):
        """Save the plot to a file without displaying it."""
        self.fig.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Plot saved to: {output_file}")


def print_summary(island_data, all_islands, archipelago_scores, objective_type=0, show_means=False):
    """Print summary statistics to console."""
    score_name = OBJECTIVE_LABELS[objective_type]['name']
    all_scores = [score for gen_scores in archipelago_scores.values()
                  for score in gen_scores]
    unique_gens = sorted(archipelago_scores.keys())
    final_gen = unique_gens[-1]

    print(f"\n{'='*60}")
    print(f"ARCHIPELAGO SUMMARY ({len(all_islands)} Islands)")
    print(f"{'='*60}")
    print(f"Objective: {score_name}")
    print(f"Total evaluations across all islands: {len(all_scores)}")
    print(f"Number of generations: {len(unique_gens)}")
    print(f"Average evaluations per generation: ~{len(all_scores) // len(unique_gens)}")
    print(f"\n{score_name} Range (Archipelago):")
    print(f"  Min: {np.min(all_scores):.4f}")
    print(f"  Max: {np.max(all_scores):.4f}")
    if show_means:
        print(f"  Overall Mean: {np.mean(all_scores):.4f}")
    print(f"  Overall Median: {np.median(all_scores):.4f}")

    final_scores = archipelago_scores[final_gen]
    print(f"\nFinal Generation (Gen {final_gen}, Archipelago):")
    if show_means:
        print(f"  Mean: {np.mean(final_scores):.4f}")
    print(f"  Median: {np.median(final_scores):.4f}")
    print(f"  Min: {np.min(final_scores):.4f}")

    print(f"\n{'='*60}")
    print("INDIVIDUAL ISLAND STATISTICS")
    print(f"{'='*60}")
    for island in all_islands:
        island_scores = [score for gen_scores in island_data[island].values()
                        for score in gen_scores]
        island_gens = sorted(island_data[island].keys())
        print(f"\nIsland {island}:")
        print(f"  Total evaluations: {len(island_scores)}")
        print(f"  Generations: {len(island_gens)}")
        print(f"  Min {score_name}: {np.min(island_scores):.4f}")
        print(f"  Max {score_name}: {np.max(island_scores):.4f}")
        if show_means:
            print(f"  Mean {score_name}: {np.mean(island_scores):.4f}")
        print(f"  Median {score_name}: {np.median(island_scores):.4f}")

        final_island_gen = island_gens[-1]
        final_island_scores = island_data[island][final_island_gen]
        print(f"  Final Gen ({final_island_gen}) - Median: {np.median(final_island_scores):.4f}, "
              f"Min: {np.min(final_island_scores):.4f}")

    print(f"\n{'='*60}\n")


def compute_file_generation_stats(filename, show_means):
    """Read a single file and return its per-generation median/min(/mean), combined
    across all islands in the file, indexed by generation number. Returns None if
    the file contains no valid data."""
    island_data, all_islands = read_data(filename)
    if len(all_islands) == 0:
        return None

    combined_scores = defaultdict(list)
    for island in all_islands:
        for gen in island_data[island]:
            combined_scores[gen].extend(island_data[island][gen])

    unique_gens, mean_scores, median_scores, min_scores, _, _ = \
        calculate_generation_stats(combined_scores)

    return {
        'gens': unique_gens,
        'median': dict(zip(unique_gens, median_scores)),
        'min': dict(zip(unique_gens, min_scores)),
        'mean': dict(zip(unique_gens, mean_scores)) if show_means else None,
    }


def aggregate_multi_run_stats(per_file_stats, show_means):
    """Aggregate per-file per-generation stats across files/runs. For each generation,
    and for each of the 'median'/'min'(/'mean') metrics, computes the median across
    files of that metric's per-generation value, along with its interquartile range.
    Generations missing from a given file are simply excluded from that generation's
    aggregation (rather than requiring every file to cover the same generations)."""
    all_gens = sorted(set(gen for stats in per_file_stats for gen in stats['gens']))
    metrics = ['median', 'min'] + (['mean'] if show_means else [])

    result = {'gens': all_gens, 'num_runs': []}
    for metric in metrics:
        result[metric] = {'center': [], 'q25': [], 'q75': []}

    for gen in all_gens:
        for metric in metrics:
            values = [stats[metric][gen] for stats in per_file_stats if gen in stats[metric]]
            result[metric]['center'].append(np.median(values))
            result[metric]['q25'].append(np.percentile(values, 25))
            result[metric]['q75'].append(np.percentile(values, 75))
        result['num_runs'].append(
            sum(1 for stats in per_file_stats if gen in stats['median']))

    return result


def print_multi_run_summary(filenames, agg_stats, objective_type=0, show_means=False):
    """Print multi-run summary statistics to console."""
    score_name = OBJECTIVE_LABELS[objective_type]['name']
    gens = agg_stats['gens']
    final_idx = -1
    final_gen = gens[final_idx]

    print(f"\n{'='*60}")
    print(f"MULTI-RUN SUMMARY ({len(filenames)} Runs)")
    print(f"{'='*60}")
    print(f"Objective: {score_name}")
    print(f"Generations: {gens[0]}-{final_gen}")
    print(f"Runs contributing to final generation: {agg_stats['num_runs'][final_idx]}")

    if show_means:
        print(f"\nFinal Generation Mean (median across runs, IQR):")
        print(f"  {agg_stats['mean']['center'][final_idx]:.4f} "
              f"[{agg_stats['mean']['q25'][final_idx]:.4f}, "
              f"{agg_stats['mean']['q75'][final_idx]:.4f}]")

    print(f"\nFinal Generation Median (median across runs, IQR):")
    print(f"  {agg_stats['median']['center'][final_idx]:.4f} "
          f"[{agg_stats['median']['q25'][final_idx]:.4f}, "
          f"{agg_stats['median']['q75'][final_idx]:.4f}]")

    print(f"\nFinal Generation Min (median across runs, IQR):")
    print(f"  {agg_stats['min']['center'][final_idx]:.4f} "
          f"[{agg_stats['min']['q25'][final_idx]:.4f}, "
          f"{agg_stats['min']['q75'][final_idx]:.4f}]")

    print(f"\n{'='*60}\n")


class MultiRunPlot:
    """Plot of aggregated per-generation median/min(/mean) across multiple runs
    (input files), each shown as a line with a shaded interquartile-range band."""

    METRIC_STYLE = {
        'mean': {'color': 'darkblue', 'marker': 'o'},
        'median': {'color': 'orange', 'marker': '^'},
        'min': {'color': 'mediumseagreen', 'marker': 's'},
    }

    def __init__(self, agg_stats, num_runs, objective_type=0, show_means=False, title=None,
                ymin=None, ymax=None, minimal=False):
        self.agg_stats = agg_stats
        self.num_runs = num_runs
        self.score_name = OBJECTIVE_LABELS[objective_type]['name']
        self.show_means = show_means
        self.title = title
        self.ymin = ymin
        self.ymax = ymax
        self.minimal = minimal

        self.setup_figure()
        self.create_plots()
        if self.ymin is not None or self.ymax is not None:
            self.ax.set_ylim(bottom=self.ymin, top=self.ymax)
        self.ax.legend(loc='upper right', framealpha=0.9, fontsize=9)
        if not self.minimal:
            self.create_stats_text()

    def setup_figure(self):
        """Set up the figure and axes layout."""
        self.fig = plt.figure(figsize=(10, 8) if self.minimal else (13, 8))

        if self.minimal:
            self.ax = self.fig.add_axes([0.10, 0.1, 0.85, 0.8])
        else:
            # Leave a narrow strip on the right for the stats text panel.
            self.ax = self.fig.add_axes([0.08, 0.1, 0.68, 0.8])

        self.ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
        self.ax.set_ylabel(self.score_name, fontsize=12, fontweight='bold')
        base_title = f'{self.num_runs} Runs (median across runs, shaded IQR)'
        full_title = '\n'.join(part for part in (self.title, base_title) if part)
        self.ax.set_title(full_title, fontsize=14, fontweight='bold')
        self.ax.grid(True, alpha=0.3, linestyle='--')

    def create_plots(self):
        """Plot the median/min(/mean) lines with shaded IQR bands."""
        gens = self.agg_stats['gens']
        metrics = ['median', 'min'] + (['mean'] if self.show_means else [])

        for metric in metrics:
            style = self.METRIC_STYLE[metric]
            data = self.agg_stats[metric]
            label = metric.capitalize()

            self.ax.fill_between(gens, data['q25'], data['q75'],
                                 color=style['color'], alpha=0.2, linewidth=0,
                                 label=f'{label} IQR')
            self.ax.plot(gens, data['center'], color=style['color'], linewidth=2.0,
                         marker=style['marker'], markersize=3,
                         label=f'{label} (n={self.num_runs} runs)')

    def create_stats_text(self):
        """Show a small stats text panel to the right of the plot."""
        gens = self.agg_stats['gens']
        final_gen = gens[-1]

        stats_text = 'Multi-Run Statistics\n'
        stats_text += '─' * 25 + '\n'
        stats_text += f'Runs: {self.num_runs}\n'
        stats_text += f'Generations: {gens[0]}-{final_gen}\n'
        stats_text += f'Final gen runs: {self.agg_stats["num_runs"][-1]}\n\n'

        if self.show_means:
            mean = self.agg_stats['mean']
            stats_text += (f'Final Mean:\n  {mean["center"][-1]:.4f} '
                           f'[{mean["q25"][-1]:.4f}, {mean["q75"][-1]:.4f}]\n')

        median = self.agg_stats['median']
        stats_text += (f'Final Median:\n  {median["center"][-1]:.4f} '
                       f'[{median["q25"][-1]:.4f}, {median["q75"][-1]:.4f}]\n')

        min_ = self.agg_stats['min']
        stats_text += (f'Final Min:\n  {min_["center"][-1]:.4f} '
                       f'[{min_["q25"][-1]:.4f}, {min_["q75"][-1]:.4f}]')

        stats_ax = self.fig.add_axes([0.80, 0.4, 0.18, 0.5])
        stats_ax.axis('off')
        stats_ax.text(0.0, 1.0, stats_text, transform=stats_ax.transAxes,
                      verticalalignment='top',
                      bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
                      fontsize=9, family='monospace')

    def show(self):
        """Display the plot."""
        plt.show()

    def save(self, output_file):
        """Save the plot to a file without displaying it."""
        self.fig.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Plot saved to: {output_file}")


def run_single_file(args):
    """Original single-file behaviour: per-island lines plus combined archipelago
    lines, with rolling averages and interactive controls."""
    input_file = args.input_files[0]

    if args.save_only:
        run_num = extract_run_number(input_file)
        if run_num is None:
            print(f"Error: --save-only requires a run number in the input filename "
                  f"(e.g. fitness-expt-N.csv), could not extract one from: {input_file}",
                  file=sys.stderr)
            sys.exit(1)

    island_data, all_islands = read_data(input_file)

    if len(all_islands) == 0:
        print("Error: No valid data found in input file", file=sys.stderr)
        sys.exit(1)

    # Combine all islands' data for archipelago-level statistics
    archipelago_scores = defaultdict(list)
    for island in all_islands:
        for gen in island_data[island]:
            archipelago_scores[gen].extend(island_data[island][gen])

    # Print summary to console
    print_summary(island_data, all_islands, archipelago_scores, args.type, args.show_means)

    # Create and show (or save) the plot
    plot = InteractivePlot(island_data, all_islands, args.type, args.show_means, args.title,
                           ymin=args.ymin, ymax=args.ymax, minimal=args.minimal,
                           one_island=args.one_island)
    if args.save_only:
        output_file = f"graph-fitness-{args.basename}_{run_num}.png"
        plot.save(output_file)
    else:
        plot.show()


def run_multi_file(args):
    """Multi-file behaviour: aggregate each file's per-generation median/min(/mean)
    across files, plotting the median across runs with a shaded IQR band."""
    per_file_stats = []
    for filename in args.input_files:
        stats = compute_file_generation_stats(filename, args.show_means)
        if stats is None:
            print(f"Warning: no valid data in {filename}, skipping", file=sys.stderr)
            continue
        per_file_stats.append(stats)

    if not per_file_stats:
        print("Error: No valid data found in any input file", file=sys.stderr)
        sys.exit(1)

    agg_stats = aggregate_multi_run_stats(per_file_stats, args.show_means)

    print_multi_run_summary(args.input_files, agg_stats, args.type, args.show_means)

    plot = MultiRunPlot(agg_stats, len(per_file_stats), args.type, args.show_means, args.title,
                        ymin=args.ymin, ymax=args.ymax, minimal=args.minimal)
    if args.save_only:
        output_file = f"graph-fitness-{args.basename}-summary.png"
        plot.save(output_file)
    else:
        plot.show()


def main():
    args = parse_arguments()

    if len(args.input_files) == 1:
        run_single_file(args)
    else:
        run_multi_file(args)


if __name__ == '__main__':
    main()
