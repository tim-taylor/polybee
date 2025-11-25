#!/usr/bin/env python3
"""
Plot EMD scores from optimization run.
Creates a scatterplot of all individual scores plus line plots for mean and min per generation,
with 10-generation rolling averages.

Dependencies:
    pip install matplotlib numpy

Or on Ubuntu/Debian:
    sudo apt install python3-matplotlib python3-numpy
"""

import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

# Read the CSV file
generations = []
emd_scores = []

with open('out3.csv', 'r') as f:
    for i, line in enumerate(f):
        # Skip header rows
        #if i < 2:
        #    continue
        line = line.strip()
        if line:
            parts = line.split(',')
            if len(parts) == 2:
                try:
                    gen = int(parts[0])
                    score = float(parts[1])
                    generations.append(gen)
                    emd_scores.append(score)
                except ValueError:
                    # Skip any malformed lines
                    continue

generations = np.array(generations)
emd_scores = np.array(emd_scores)

# Calculate mean and min EMD score per generation
gen_scores = defaultdict(list)
for gen, score in zip(generations, emd_scores):
    gen_scores[gen].append(score)

unique_gens = sorted(gen_scores.keys())
mean_scores = [np.mean(gen_scores[g]) for g in unique_gens]
min_scores = [np.min(gen_scores[g]) for g in unique_gens]

# Calculate 10-generation rolling averages
window_size = 10
mean_rolling = np.convolve(mean_scores, np.ones(window_size)/window_size, mode='valid')
min_rolling = np.convolve(min_scores, np.ones(window_size)/window_size, mode='valid')
# Adjust generation indices for rolling averages (centered on window)
rolling_gens = unique_gens[window_size-1:]

# Create the plot
fig, ax = plt.subplots(figsize=(12, 7))

# Scatterplot of all individual EMD scores
ax.scatter(generations, emd_scores, alpha=0.3, s=20, c='lightblue',
           label='Individual scores', edgecolors='none')

# Line plot of mean EMD score per generation
ax.plot(unique_gens, mean_scores,
        color='blue', linewidth=1.5, alpha=0.5, label='Mean EMD per generation',
        marker='o', markersize=3)

# Line plot of min EMD score per generation
ax.plot(unique_gens, min_scores,
        color='red', linewidth=1.5, alpha=0.5, label='Min EMD per generation',
        marker='s', markersize=3)

# Rolling average plots (10 generations)
ax.plot(rolling_gens, mean_rolling,
        color='darkblue', linewidth=2.5, label='Mean EMD (10-gen rolling avg)',
        linestyle='-')

ax.plot(rolling_gens, min_rolling,
        color='darkred', linewidth=2.5, label='Min EMD (10-gen rolling avg)',
        linestyle='-')

# Customize the plot
ax.set_xlabel('Generation', fontsize=12, fontweight='bold')
ax.set_ylabel('EMD Score', fontsize=12, fontweight='bold')
ax.set_title('EMD Scores Across Generations', fontsize=14, fontweight='bold')
ax.legend(loc='best', framealpha=0.9)
ax.grid(True, alpha=0.3, linestyle='--')

# Add some statistics as text
final_gen = unique_gens[-1]
final_mean = mean_scores[-1]
final_min = min_scores[-1]
overall_min = np.min(emd_scores)

stats_text = f'Final Generation: {final_gen}\n'
stats_text += f'Final Mean: {final_mean:.3f}\n'
stats_text += f'Final Min: {final_min:.3f}\n'
stats_text += f'Overall Min: {overall_min:.3f}'

ax.text(0.02, 0.98, stats_text, transform=ax.transAxes,
        verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8),
        fontsize=10, family='monospace')

plt.tight_layout()

# Print summary statistics
print("\nSummary Statistics:")
print(f"Total individuals evaluated: {len(emd_scores)}")
print(f"Number of generations: {final_gen + 1}")
print(f"Population size per generation: ~{len(emd_scores) // (final_gen + 1)}")
print(f"\nEMD Score Range:")
print(f"  Min: {overall_min:.4f}")
print(f"  Max: {np.max(emd_scores):.4f}")
print(f"  Overall Mean: {np.mean(emd_scores):.4f}")
print(f"\nFinal Generation (Gen {final_gen}):")
print(f"  Mean: {final_mean:.4f}")
print(f"  Min: {final_min:.4f}")
print(f"  Mean (10-gen rolling avg): {mean_rolling[-1]:.4f}")
print(f"  Min (10-gen rolling avg): {min_rolling[-1]:.4f}")

plt.show()
