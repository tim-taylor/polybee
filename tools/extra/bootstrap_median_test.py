#!/usr/bin/env python3
"""
Bootstrap significance test for the difference in median fitness between
each evolve condition's 50 champion-fitness values and the baseline
condition's 5000 per-run fitness values (same data as calc_fitness_stats.sh).

For each evolve condition, this computes:
  - observed statistic: median(condition) - median(baseline)
  - a 95% percentile bootstrap confidence interval for that statistic,
    resampling each of the two samples independently (each at its own size:
    50 for the evolve condition, 5000 for baseline), via scipy.stats.bootstrap
  - a two-sided bootstrap hypothesis-test p-value, computed by recentring
    both samples to share a common median (subtracting each sample's own
    median, so H0 "equal medians" holds by construction), resampling
    independently from the recentred samples, and counting how often the
    resulting null difference is at least as extreme (in absolute value) as
    the actually observed difference (the bootstrap "shift" test of Davison
    & Hinkley, Bootstrap Methods and Their Application, 1997, ch. 4)

Bonferroni correction is then applied across the 3 evolve-vs-baseline
comparisons, to control the family-wise error rate across the 3 tests
sharing the same baseline sample.

The 50-vs-5000 sample-size mismatch needs no special handling: each group is
resampled at its own size throughout, which is exactly what treating each
group as an independent random sample from its own population requires.

Usage:
    python3 bootstrap_median_test.py [--n-resamples N] [--seed S] [--alpha A]

Run from the same directory as calc_fitness_stats.sh (expects the same
per-condition fitness-data-agg/champion-fitnesses-*.csv layout, i.e. one
CONDITION/fitness-data-agg/champion-fitnesses-CONDITION.csv per condition,
with fitness in the second column).
"""

import argparse
import csv
import sys

import numpy as np
from scipy.stats import bootstrap

BASELINE = "baseline-runs-2000its"
CONDITIONS = [
    "evolve-10B-400gen-400pop-100epi-2000its",
    "evolve-20X-400gen-400pop-100epi-2000its",
    "evolve-20X-10B-400gen-400pop-100epi-2000its",
]

BATCH_SIZE = 1000  # caps peak memory use during bootstrap resampling


def load_fitnesses(cond):
    path = f"{cond}/fitness-data-agg/champion-fitnesses-{cond}.csv"
    try:
        with open(path) as f:
            return np.array([float(row[1]) for row in csv.reader(f) if row])
    except FileNotFoundError:
        sys.exit(f"Error: {path} not found -- run this script from the 'analysed' "
                 "directory containing per-condition fitness-data-agg/ subdirs.")


def median_diff_stat(a, b, axis=-1):
    return np.median(a, axis=axis) - np.median(b, axis=axis)


def bootstrap_shift_test(sample_a, sample_b, observed_diff, n_resamples, rng):
    """Two-sided bootstrap hypothesis-test p-value for median(a) == median(b)."""
    a_centred = sample_a - np.median(sample_a)
    b_centred = sample_b - np.median(sample_b)
    na, nb = len(a_centred), len(b_centred)

    exceed = 0
    remaining = n_resamples
    while remaining > 0:
        batch = min(BATCH_SIZE, remaining)
        a_res = rng.choice(a_centred, size=(batch, na), replace=True)
        b_res = rng.choice(b_centred, size=(batch, nb), replace=True)
        null_diffs = np.median(a_res, axis=1) - np.median(b_res, axis=1)
        exceed += int(np.sum(np.abs(null_diffs) >= abs(observed_diff)))
        remaining -= batch

    # +1 smoothing avoids reporting p=0 (Davison & Hinkley 1997, sec 4.2)
    return (exceed + 1) / (n_resamples + 1)


def bonferroni(pvalues):
    """Return Bonferroni-adjusted p-values (p * m, capped at 1)."""
    m = len(pvalues)
    return np.minimum(pvalues * m, 1.0)


def main():
    parser = argparse.ArgumentParser(
        description="Bootstrap test of whether each evolve condition's median "
                    "champion fitness differs significantly from the baseline's.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('--n-resamples', type=int, default=10000,
                       help='Bootstrap resamples used for both the CI and the '
                            'p-value test (default: 10000)')
    parser.add_argument('--seed', type=int, default=12345,
                       help='RNG seed, for reproducibility (default: 12345)')
    parser.add_argument('--alpha', type=float, default=0.05,
                       help='Significance level for the CI and the Bonferroni '
                            'test (default: 0.05)')
    args = parser.parse_args()

    rng = np.random.default_rng(args.seed)

    baseline = load_fitnesses(BASELINE)

    results = []
    for cond in CONDITIONS:
        sample = load_fitnesses(cond)
        observed = median_diff_stat(sample, baseline)

        ci = bootstrap(
            (sample, baseline), median_diff_stat, method='percentile',
            n_resamples=args.n_resamples, vectorized=True, paired=False,
            confidence_level=1.0 - args.alpha, batch=BATCH_SIZE, random_state=rng,
        )

        pval = bootstrap_shift_test(sample, baseline, observed, args.n_resamples, rng)

        results.append({
            'condition': cond,
            'n': len(sample),
            'median': np.median(sample),
            'baseline_median': np.median(baseline),
            'diff': observed,
            'ci_lo': ci.confidence_interval.low,
            'ci_hi': ci.confidence_interval.high,
            'p_raw': pval,
        })

    adjusted = bonferroni(np.array([r['p_raw'] for r in results]))
    for r, p_bonf in zip(results, adjusted):
        r['p_bonf'] = p_bonf

    ci_pct = round((1.0 - args.alpha) * 100)
    header = ["condition", "n", "median", "baseline_median", "diff",
              f"ci{ci_pct}_lo", f"ci{ci_pct}_hi", "p_raw", "p_bonferroni",
              f"significant_at_{args.alpha}"]
    print(",".join(header))
    for r in results:
        significant = "yes" if r['p_bonf'] < args.alpha else "no"
        print(",".join([
            r['condition'], str(r['n']),
            f"{r['median']:.5f}", f"{r['baseline_median']:.5f}", f"{r['diff']:.5f}",
            f"{r['ci_lo']:.5f}", f"{r['ci_hi']:.5f}",
            f"{r['p_raw']:.5f}", f"{r['p_bonf']:.5f}", significant,
        ]))


if __name__ == '__main__':
    main()
