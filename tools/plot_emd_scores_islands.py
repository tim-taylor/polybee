#!/usr/bin/env python3
"""
Plot EMD scores from archipelago optimization run with multiple islands.

By default, combines results across all islands and plots overall archipelago statistics.
With --all flag, plots individual island results as well as overall archipelago results.

Input CSV format: island_number,generation_number,median_emd_score

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy

Usage:
    ./plot_emd_scores_islands.py <input_csv_file>
    ./plot_emd_scores_islands.py --all <input_csv_file>
"""

import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
import sys
import argparse

# Parse command line arguments
parser = argparse.ArgumentParser(description='Plot EMD scores from archipelago optimization run')
parser.add_argument('input_file', help='Input CSV file (island,generation,emd_score)')
parser.add_argument('--all', action='store_true', dest='plot_all',
                    help='Plot individual island results in addition to overall archipelago results')
args = parser.parse_args()

# Read the CSV file
# Data structure: island_data[island_num][generation] = [list of scores]
island_data = defaultdict(lambda: defaultdict(list))
all_islands = set()

with open(args.input_file, 'r') as f:
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
                    # Skip any malformed lines
                    continue

all_islands = sorted(all_islands)
num_islands = len(all_islands)

if num_islands == 0:
    print("Error: No valid data found in input file", file=sys.stderr)
    sys.exit(1)


def calculate_generation_stats(data_dict):
    """Calculate mean, median, and min scores per generation from a data dictionary."""
    unique_gens = sorted(data_dict.keys())
    mean_scores = [np.mean(data_dict[g]) for g in unique_gens]
    median_scores = [np.median(data_dict[g]) for g in unique_gens]
    min_scores = [np.min(data_dict[g]) for g in unique_gens]
    return unique_gens, mean_scores, median_scores, min_scores


def calculate_rolling_average(scores, window_size=10):
    """Calculate rolling average if enough data points exist."""
    if len(scores) >= window_size:
        return np.convolve(scores, np.ones(window_size)/window_size, mode='valid')
    return None


def plot_island_results(ax, island_num, gen_scores, color, show_individuals=False):
    """Plot results for a single island."""
    unique_gens, mean_scores, median_scores, min_scores = calculate_generation_stats(gen_scores)

    # Optionally plot individual scores
    if show_individuals:
        all_gens = []
        all_scores = []
        for gen in unique_gens:
            all_gens.extend([gen] * len(gen_scores[gen]))
            all_scores.extend(gen_scores[gen])
        ax.scatter(all_gens, all_scores, alpha=0.2, s=15, c=color,
                   edgecolors='none')

    # Plot mean, median, and min
    ax.plot(unique_gens, mean_scores, color=color, linewidth=1.0, alpha=0.4,
            linestyle='--', marker='o', markersize=2)
    ax.plot(unique_gens, median_scores, color=color, linewidth=1.5, alpha=0.6,
            label=f'Island {island_num}', marker='^', markersize=3)
    ax.plot(unique_gens, min_scores, color=color, linewidth=1.0, alpha=0.4,
            linestyle=':', marker='s', markersize=2)

    # Rolling averages
    window_size = 10
    median_rolling = calculate_rolling_average(median_scores, window_size)
    if median_rolling is not None:
        rolling_gens = unique_gens[window_size-1:]
        ax.plot(rolling_gens, median_rolling, color=color, linewidth=2.0,
                linestyle='-', alpha=0.8)

    return unique_gens, mean_scores, median_scores, min_scores


def plot_archipelago_results(ax, archipelago_scores, use_original_colors=False, label_prefix='Archipelago'):
    """Plot combined results for the entire archipelago.

    If use_original_colors=True, uses the color scheme from the original plot_emd_scores.py.
    Otherwise uses black for all lines (suitable for overlaying with island results).
    """
    unique_gens, mean_scores, median_scores, min_scores = calculate_generation_stats(archipelago_scores)

    # Plot individual scores
    all_gens = []
    all_scores = []
    for gen in unique_gens:
        all_gens.extend([gen] * len(archipelago_scores[gen]))
        all_scores.extend(archipelago_scores[gen])

    if use_original_colors:
        # Use original color scheme
        ax.scatter(all_gens, all_scores, alpha=0.3, s=20, c='lightblue',
                   label='Individual scores', edgecolors='none')

        # Plot mean, median, and min per generation with original colors
        ax.plot(unique_gens, mean_scores, color='blue', linewidth=1.5, alpha=0.5,
                label='Mean EMD per generation', marker='o', markersize=3)
        ax.plot(unique_gens, median_scores, color='green', linewidth=1.5, alpha=0.5,
                label='Median EMD per generation', marker='^', markersize=3)
        ax.plot(unique_gens, min_scores, color='red', linewidth=1.5, alpha=0.5,
                label='Min EMD per generation', marker='s', markersize=3)

        # Calculate and plot rolling averages with original colors
        window_size = 10
        mean_rolling = calculate_rolling_average(mean_scores, window_size)
        median_rolling = calculate_rolling_average(median_scores, window_size)
        min_rolling = calculate_rolling_average(min_scores, window_size)

        if median_rolling is not None:
            rolling_gens = unique_gens[window_size-1:]
            ax.plot(rolling_gens, mean_rolling, color='darkblue', linewidth=2.5,
                    label='Mean EMD (10-gen rolling avg)', linestyle='-')
            ax.plot(rolling_gens, median_rolling, color='darkgreen', linewidth=2.5,
                    label='Median EMD (10-gen rolling avg)', linestyle='-')
            ax.plot(rolling_gens, min_rolling, color='darkred', linewidth=2.5,
                    label='Min EMD (10-gen rolling avg)', linestyle='-')
    else:
        # Use black for overlaying with island results
        ax.scatter(all_gens, all_scores, alpha=0.3, s=20, c='lightgray',
                   label='Individual scores', edgecolors='none')

        # Plot mean, median, and min per generation
        ax.plot(unique_gens, mean_scores, color='black', linewidth=1.5, alpha=0.5,
                label=f'{label_prefix} Mean', marker='o', markersize=3, linestyle='--')
        ax.plot(unique_gens, median_scores, color='black', linewidth=1.5, alpha=0.5,
                label=f'{label_prefix} Median', marker='^', markersize=3)
        ax.plot(unique_gens, min_scores, color='black', linewidth=1.5, alpha=0.5,
                label=f'{label_prefix} Min', marker='s', markersize=3, linestyle=':')

        # Calculate and plot rolling averages
        window_size = 10
        mean_rolling = calculate_rolling_average(mean_scores, window_size)
        median_rolling = calculate_rolling_average(median_scores, window_size)
        min_rolling = calculate_rolling_average(min_scores, window_size)

        if median_rolling is not None:
            rolling_gens = unique_gens[window_size-1:]
            ax.plot(rolling_gens, mean_rolling, color='black', linewidth=2.5,
                    label=f'{label_prefix} Mean (10-gen rolling)', linestyle='--')
            ax.plot(rolling_gens, median_rolling, color='black', linewidth=2.5,
                    label=f'{label_prefix} Median (10-gen rolling)', linestyle='-')
            ax.plot(rolling_gens, min_rolling, color='black', linewidth=2.5,
                    label=f'{label_prefix} Min (10-gen rolling)', linestyle=':')

    return unique_gens, mean_scores, median_scores, min_scores, mean_rolling, median_rolling, min_rolling


# Combine all islands' data for archipelago-level statistics
archipelago_scores = defaultdict(list)
for island in all_islands:
    for gen in island_data[island]:
        archipelago_scores[gen].extend(island_data[island][gen])

# Create plots based on --all flag
if args.plot_all:
    # Plot all islands and archipelago results on a single plot
    fig, ax = plt.subplots(figsize=(14, 8))

    # Generate distinct colors for each island
    colors = plt.cm.tab10(np.linspace(0, 1, num_islands))

    # Plot each island
    for idx, island in enumerate(all_islands):
        unique_gens, mean_scores, median_scores, min_scores = plot_island_results(
            ax, island, island_data[island], colors[idx], show_individuals=False)

    # Plot archipelago results with thicker lines (use black to distinguish from islands)
    unique_gens, mean_scores, median_scores, min_scores, mean_rolling, median_rolling, min_rolling = \
        plot_archipelago_results(ax, archipelago_scores, use_original_colors=False, label_prefix='Overall')

    ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
    ax.set_ylabel('EMD Score', fontsize=12, fontweight='bold')
    ax.set_title(f'Archipelago ({num_islands} Islands) - EMD Scores Across Generations',
                 fontsize=14, fontweight='bold')
    ax.legend(loc='best', framealpha=0.9, fontsize=9, ncol=2)
    ax.grid(True, alpha=0.3, linestyle='--')

else:
    # Plot only archipelago results using original color scheme
    fig, ax = plt.subplots(figsize=(12, 7))
    unique_gens, mean_scores, median_scores, min_scores, mean_rolling, median_rolling, min_rolling = \
        plot_archipelago_results(ax, archipelago_scores, use_original_colors=True)

    ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
    ax.set_ylabel('EMD Score', fontsize=12, fontweight='bold')
    ax.set_title(f'Archipelago ({num_islands} Islands Combined) - EMD Scores Across Generations',
                 fontsize=14, fontweight='bold')
    ax.legend(loc='best', framealpha=0.9)
    ax.grid(True, alpha=0.3, linestyle='--')

# Add overall statistics text for archipelago
final_gen = unique_gens[-1]
final_mean = mean_scores[-1]
final_median = median_scores[-1]
final_min = min_scores[-1]
all_scores = [score for gen_scores in archipelago_scores.values() for score in gen_scores]
overall_min = np.min(all_scores)

stats_text = f'Final Generation: {final_gen}\n'
stats_text += f'Final Mean: {final_mean:.3f}\n'
stats_text += f'Final Median: {final_median:.3f}\n'
stats_text += f'Final Min: {final_min:.3f}\n'
stats_text += f'Overall Min: {overall_min:.3f}'

# Get the axes object (only one axes in both cases now)
ax = fig.axes[0]

ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
        verticalalignment='top',
        bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
        fontsize=10, family='monospace')

plt.tight_layout()

# Print summary statistics
print(f"\n{'='*60}")
print(f"ARCHIPELAGO SUMMARY ({num_islands} Islands)")
print(f"{'='*60}")
print(f"Total evaluations across all islands: {len(all_scores)}")
print(f"Number of generations: {final_gen + 1}")
print(f"Average evaluations per generation: ~{len(all_scores) // (final_gen + 1)}")
print(f"\nEMD Score Range (Archipelago):")
print(f"  Min: {overall_min:.4f}")
print(f"  Max: {np.max(all_scores):.4f}")
print(f"  Overall Mean: {np.mean(all_scores):.4f}")
print(f"  Overall Median: {np.median(all_scores):.4f}")
print(f"\nFinal Generation (Gen {final_gen}, Archipelago):")
print(f"  Mean: {final_mean:.4f}")
print(f"  Median: {final_median:.4f}")
print(f"  Min: {final_min:.4f}")
if mean_rolling is not None:
    print(f"  Mean (10-gen rolling avg): {mean_rolling[-1]:.4f}")
    print(f"  Median (10-gen rolling avg): {median_rolling[-1]:.4f}")
    print(f"  Min (10-gen rolling avg): {min_rolling[-1]:.4f}")

# Print per-island statistics if --all flag is used
if args.plot_all:
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

        # Final generation stats for this island
        final_island_gen = island_gens[-1]
        final_island_scores = island_data[island][final_island_gen]
        print(f"  Final Gen ({final_island_gen}) - Median: {np.median(final_island_scores):.4f}, "
              f"Min: {np.min(final_island_scores):.4f}")

print(f"\n{'='*60}\n")

plt.show()
