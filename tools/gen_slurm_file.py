#!/usr/bin/env python3
"""
Generate a SLURM batch file for a PolyBee experiment.

Usage:
    ./gen-slurm-file.py <experiment_name> [-n <replicates>] [-t <D-HH:MM:SS>]
"""

import argparse
import re
import sys
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate a SLURM batch file for a PolyBee experiment')
    parser.add_argument('experiment_name',
                        help='Name of the experiment')
    parser.add_argument('-n', '--replicates', type=int, default=1,
                        metavar='N',
                        help='Number of replicate runs (default: 1, minimum: 1)')
    parser.add_argument('-t', '--time', default='1-00:00:00',
                        metavar='D-HH:MM:SS',
                        help='Maximum wall time (default: 1-00:00:00)')
    return parser.parse_args()


def validate_time(time_str):
    if not re.fullmatch(r'\d+-\d{2}:\d{2}:\d{2}', time_str):
        print(f"Error: time must be in D-HH:MM:SS format, got: {time_str}", file=sys.stderr)
        sys.exit(1)


def generate_slurm(name, replicates, time_limit):
    return f"""\
#!/bin/bash
#SBATCH --job-name="{name}"
#SBATCH --output="out-{name}.txt"
#SBATCH --array=1-{replicates}
#SBATCH --time={time_limit}
#SBATCH --cpus-per-task=1
#SBATCH --mem=1G
#SBATCH --constraint=intel
#SBATCH --account=tf31
#SBATCH --mail-user=tim.taylor@gmail.com
#SBATCH --mail-type=END,FAIL,REQUEUE,INVALID_DEPEND,TIME_LIMIT,TIME_LIMIT_80,TIME_LIMIT_90

#############################################################

PB_BASE_DIR=/projects/tf31/projects/polybee
PB_EXEC=${{PB_BASE_DIR}}/build/bin/release/polybee

EXPT_NAME="{name}"
EXPT_DIR="${{PB_BASE_DIR}}/m3/expts/${{EXPT_NAME}}"
CONFIG_FILE="${{EXPT_DIR}}/${{EXPT_NAME}}.cfg"

GCC_MODULE="linux-rocky9-cascadelake/gcc/15.2.0-gcc-11.5.0"

THIS_JOB_ID="${{SLURM_ARRAY_JOB_ID}}_${{SLURM_ARRAY_TASK_ID}}"

RUN_CMD="${{PB_EXEC}} -c ${{CONFIG_FILE}} --log-filename-prefix ${{EXPT_NAME}}-${{THIS_JOB_ID}}"

#############################################################

echo -n "*** Job ${{THIS_JOB_ID}} started at "; date

module load ${{GCC_MODULE}}
echo "Loaded module ${{GCC_MODULE}}"

cd ${{PB_BASE_DIR}}

echo "Running command: ${{RUN_CMD}}"
${{RUN_CMD}}

echo "Moving output files to ${{EXPT_DIR}}"
mv ${{PB_BASE_DIR}}/output/${{EXPT_NAME}}-${{SLURM_ARRAY_JOB_ID}}* ${{EXPT_DIR}}

echo -n "*** Job ${{THIS_JOB_ID}} ended at "; date

#############################################################
"""


def main():
    args = parse_args()

    if args.replicates < 1:
        print(f"Error: number of replicates must be 1 or greater, got: {args.replicates}",
              file=sys.stderr)
        sys.exit(1)

    validate_time(args.time)

    outfile = Path(args.experiment_name + '.slurm')

    if outfile.exists():
        response = input(f"File '{outfile}' already exists. Overwrite? [y/N] ").strip().lower()
        if response not in ('y', 'yes'):
            print("Aborted.")
            sys.exit(0)

    content = generate_slurm(args.experiment_name, args.replicates, args.time)
    outfile.write_text(content)
    print(f"Written: {outfile}")


if __name__ == '__main__':
    main()
