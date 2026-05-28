#!/bin/bash
#SBATCH --job-name="test-evolve-barriers-1"
#SBATCH --account=tf31
#SBATCH --time=1-12:00:00
#SBATCH --cpus-per-task=1
#SBATCH --mem=8G
#SBATCH --array=1-3
#SBATCH --mail-user=tim.taylor@monash.edu
#SBATCH --mail-type=ALL

PB_BASE_DIR=/projects/tf31/projects/polybee
PB_EXEC=${PB_BASE_DIR}/build/bin/release/polybee

EXPT_NAME="test-evolve-barriers"
EXPT_FILE="${PB_BASE_DIR}/m3/expts/${EXPT_NAME}/"
CONFIG_FILE="${EXPT_DIR}/${EXPT_NAME}.cfg"

THIS_JOB_ID="${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}"

date
echo "Job ${THIS_JOB_ID} has started!"

module load linux-rocky9-sapphirerapids/gcc
$PB_EXEC -c ${CONFIG_FILE} --log-filename-prefix "${EXPT_NAME}-${THIS_JOB_ID}"

date
echo "Job ${THIS_JOB_ID} has ended!"
