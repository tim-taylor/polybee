#!/usr/bin/env python3
"""Extract champion fitness values from Slurm run output files into a sorted CSV.

What it does:
    Takes one or more Slurm run output files, named out-*_N.txt (as written by
    the #SBATCH --output="out-{name}-%A_%a.txt" pattern in gen_slurm_file.py,
    where N is the Slurm array task ID), and finds the champion fitness in
    each one: the last line matching "Champion fitness: FITNESS" (the bare,
    non-bracketed form written once at the end of a run — see
    PolyBeeEvolve.cpp).

    N is taken from the input filename: the run number between the last
    underscore and the .txt extension (e.g. out-myexpt-58020874_34.txt -> 34).

    Files with no matching "Champion fitness:" line, or whose filename
    doesn't end in _N.txt, are skipped with a warning.

Usage:
    ./gen_champion_fitness_csv.py OUTFILE [OUTFILE ...] [--basename NAME]

    OUTFILE          One or more out-*_N.txt Slurm output files (glob-expand
                     with your shell to pass many, e.g. out-myexpt-*_*.txt).
    --basename NAME  Basename for the output CSV file (default: output).
                     Writes champion-fitnesses-NAME.csv in the current
                     directory.

Output:
    A CSV file with one line per input file: N,FITNESS
    Sorted by fitness ascending (best/lowest fitness first).

Example:
    ./gen_champion_fitness_csv.py out-myexpt-58020874_*.txt --basename myexpt
    # -> writes champion-fitnesses-myexpt.csv
"""

import argparse
import os
import re
import sys


RUN_NUM_RE = re.compile(r'_(\d+)\.txt$')
CHAMPION_FITNESS_RE = re.compile(r'^Champion fitness: ([+-]?\d+\.\d+)\s*$')


def extract_run_number(filepath):
    """Return the run number N from a out-*_N.txt filename, or None."""
    match = RUN_NUM_RE.search(os.path.basename(filepath))
    if not match:
        return None
    return int(match.group(1))


def extract_champion_fitness(filepath):
    """Return the last bare 'Champion fitness: F' value in a file, or None."""
    fitness = None
    with open(filepath) as f:
        for line in f:
            match = CHAMPION_FITNESS_RE.match(line.rstrip('\n'))
            if match:
                fitness = float(match.group(1))
    return fitness


def main():
    parser = argparse.ArgumentParser(
        description='Extract champion fitness values from Slurm run output files into a sorted CSV.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s out-myexpt-58020874_*.txt
  %(prog)s out-myexpt-58020874_*.txt --basename myexpt
        """
    )
    parser.add_argument('outfiles', nargs='+', help='out-*_N.txt Slurm output file(s) to process')
    parser.add_argument(
        '--basename', default='output',
        help='Basename for the output CSV file (default: output)'
    )
    args = parser.parse_args()

    results = []
    for filepath in args.outfiles:
        if not os.path.isfile(filepath):
            print(f"Warning: file not found, skipping: {filepath}", file=sys.stderr)
            continue

        run_num = extract_run_number(filepath)
        if run_num is None:
            print(f"Warning: could not extract run number from filename, skipping: {filepath}",
                  file=sys.stderr)
            continue

        fitness = extract_champion_fitness(filepath)
        if fitness is None:
            print(f"Warning: no 'Champion fitness:' line found, skipping: {filepath}",
                  file=sys.stderr)
            continue

        results.append((run_num, fitness))

    if not results:
        print("Error: no champion fitness values found in any input file", file=sys.stderr)
        sys.exit(1)

    results.sort(key=lambda r: r[1])

    out_path = f"champion-fitnesses-{args.basename}.csv"
    with open(out_path, 'w') as f:
        for run_num, fitness in results:
            f.write(f"{run_num},{fitness:.5f}\n")

    print(f"Wrote {out_path}  ({len(results)} runs, best fitness {results[0][1]:.5f})")


if __name__ == '__main__':
    main()
