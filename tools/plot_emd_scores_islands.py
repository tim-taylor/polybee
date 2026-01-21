#!/usr/bin/env python3
"""
Plot EMD scores from archipelago optimization run with multiple islands.

Interactive UI allows toggling display of:
- Individual island results
- Overall archipelago results
- Different metrics (mean, median, min EMD, individual scores)

Input CSV format: island_number,generation_number,median_emd_score

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./plot_emd_scores_islands.py <input_csv_file>
"""

import matplotlib.pyplot as plt
from matplotlib.widgets import CheckButtons, Button
import numpy as np
from collections import defaultdict
import sys
import argparse


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='Plot EMD scores from archipelago optimization run')
    parser.add_argument('input_file', help='Input CSV file (island,generation,emd_score)')
    return parser.parse_args()


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

    def __init__(self, island_data, all_islands):
        self.island_data = island_data
        self.all_islands = all_islands
        self.num_islands = len(all_islands)

        # Combine all islands' data for archipelago-level statistics
        self.archipelago_scores = defaultdict(list)
        for island in all_islands:
            for gen in island_data[island]:
                self.archipelago_scores[gen].extend(island_data[island][gen])

        # Generate distinct colors for each island
        self.colors = plt.cm.tab10(np.linspace(0, 1, max(10, self.num_islands)))

        # Store plot elements for toggling
        self.island_plots = {}  # island -> {'mean': line, 'median': line, 'min': line, 'individual': scatter, 'rolling': line}
        self.archipelago_plots = {}  # metric -> plot element

        # Visibility state
        self.island_visible = {island: True for island in all_islands}
        self.island_visible['archipelago'] = True
        self.metric_visible = {
            'mean': True,
            'median': True,
            'min': True,
            'individual': False,  # Off by default as it can be cluttered
            'rolling': True
        }

        self.setup_figure()
        self.create_plots()
        self.create_controls()
        self.update_stats_text()

    def setup_figure(self):
        """Set up the figure and axes layout."""
        self.fig = plt.figure(figsize=(16, 9))

        # Main plot area (leave room for controls on the right)
        self.ax = self.fig.add_axes([0.08, 0.1, 0.55, 0.8])

        self.ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
        self.ax.set_ylabel('EMD Score', fontsize=12, fontweight='bold')
        self.ax.set_title(f'Archipelago ({self.num_islands} Islands) - EMD Scores',
                         fontsize=14, fontweight='bold')
        self.ax.grid(True, alpha=0.3, linestyle='--')

    def create_plots(self):
        """Create all plot elements (initially visible based on defaults)."""
        # Plot each island
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

        # Mean line
        mean_line, = self.ax.plot(unique_gens, mean_scores, color=color, linewidth=1.0,
                                  alpha=0.4, linestyle='--', marker='o', markersize=2,
                                  visible=self.metric_visible['mean'])
        plots['mean'] = mean_line

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

        # Individual scores (scatter)
        scatter = self.ax.scatter(all_gens, all_scores, alpha=0.15, s=20, c='gray',
                                  edgecolors='none', visible=self.metric_visible['individual'])
        plots['individual'] = scatter

        # Mean line
        mean_line, = self.ax.plot(unique_gens, mean_scores, color='black', linewidth=1.5,
                                  alpha=0.5, linestyle='--', marker='o', markersize=3,
                                  label='Archipelago Mean', visible=self.metric_visible['mean'])
        plots['mean'] = mean_line

        # Median line
        median_line, = self.ax.plot(unique_gens, median_scores, color='black', linewidth=1.5,
                                    alpha=0.5, marker='^', markersize=3,
                                    label='Archipelago Median', visible=self.metric_visible['median'])
        plots['median'] = median_line

        # Min line
        min_line, = self.ax.plot(unique_gens, min_scores, color='black', linewidth=1.5,
                                 alpha=0.5, linestyle=':', marker='s', markersize=3,
                                 label='Archipelago Min', visible=self.metric_visible['min'])
        plots['min'] = min_line

        # Rolling averages
        window_size = 10
        mean_rolling = calculate_rolling_average(mean_scores, window_size)
        median_rolling = calculate_rolling_average(median_scores, window_size)
        min_rolling = calculate_rolling_average(min_scores, window_size)

        if median_rolling is not None:
            rolling_gens = unique_gens[window_size-1:]

            mean_roll_line, = self.ax.plot(rolling_gens, mean_rolling, color='darkblue',
                                           linewidth=2.5, linestyle='--',
                                           label='Archipelago Mean (rolling)',
                                           visible=self.metric_visible['rolling'] and self.metric_visible['mean'])
            plots['mean_rolling'] = mean_roll_line

            median_roll_line, = self.ax.plot(rolling_gens, median_rolling, color='darkgreen',
                                             linewidth=2.5, linestyle='-',
                                             label='Archipelago Median (rolling)',
                                             visible=self.metric_visible['rolling'] and self.metric_visible['median'])
            plots['median_rolling'] = median_roll_line

            min_roll_line, = self.ax.plot(rolling_gens, min_rolling, color='darkred',
                                          linewidth=2.5, linestyle=':',
                                          label='Archipelago Min (rolling)',
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
        metric_labels = ['Mean EMD', 'Median EMD', 'Min EMD', 'Individual Scores', 'Rolling Avg']
        metric_keys = ['mean', 'median', 'min', 'individual', 'rolling']
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
            'Mean EMD': 'mean',
            'Median EMD': 'median',
            'Min EMD': 'min',
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


def print_summary(island_data, all_islands, archipelago_scores):
    """Print summary statistics to console."""
    all_scores = [score for gen_scores in archipelago_scores.values()
                  for score in gen_scores]
    unique_gens = sorted(archipelago_scores.keys())
    final_gen = unique_gens[-1]

    print(f"\n{'='*60}")
    print(f"ARCHIPELAGO SUMMARY ({len(all_islands)} Islands)")
    print(f"{'='*60}")
    print(f"Total evaluations across all islands: {len(all_scores)}")
    print(f"Number of generations: {final_gen + 1}")
    print(f"Average evaluations per generation: ~{len(all_scores) // (final_gen + 1)}")
    print(f"\nEMD Score Range (Archipelago):")
    print(f"  Min: {np.min(all_scores):.4f}")
    print(f"  Max: {np.max(all_scores):.4f}")
    print(f"  Overall Mean: {np.mean(all_scores):.4f}")
    print(f"  Overall Median: {np.median(all_scores):.4f}")

    final_scores = archipelago_scores[final_gen]
    print(f"\nFinal Generation (Gen {final_gen}, Archipelago):")
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
        print(f"  Min EMD: {np.min(island_scores):.4f}")
        print(f"  Max EMD: {np.max(island_scores):.4f}")
        print(f"  Mean EMD: {np.mean(island_scores):.4f}")
        print(f"  Median EMD: {np.median(island_scores):.4f}")

        final_island_gen = island_gens[-1]
        final_island_scores = island_data[island][final_island_gen]
        print(f"  Final Gen ({final_island_gen}) - Median: {np.median(final_island_scores):.4f}, "
              f"Min: {np.min(final_island_scores):.4f}")

    print(f"\n{'='*60}\n")


def main():
    args = parse_arguments()
    island_data, all_islands = read_data(args.input_file)

    if len(all_islands) == 0:
        print("Error: No valid data found in input file", file=sys.stderr)
        sys.exit(1)

    # Combine all islands' data for archipelago-level statistics
    archipelago_scores = defaultdict(list)
    for island in all_islands:
        for gen in island_data[island]:
            archipelago_scores[gen].extend(island_data[island][gen])

    # Print summary to console
    print_summary(island_data, all_islands, archipelago_scores)

    # Create and show interactive plot
    plot = InteractivePlot(island_data, all_islands)
    plot.show()


if __name__ == '__main__':
    main()
