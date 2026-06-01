#!/bin/bash
#SBATCH --job-name="test-evolve-barriers-1"
#SBATCH --array=1-3
#SBATCH --time=1-12:00:00
#SBATCH --cpus-per-task=1
#SBATCH --mem=8G
#SBATCH --constraint=intel
#SBATCH --account=tf31
#SBATCH --mail-user=tim.taylor@monash.edu
#SBATCH --mail-type=ALL

#############################################################

PB_BASE_DIR=/projects/tf31/projects/polybee
PB_EXEC=${PB_BASE_DIR}/build/bin/release/polybee

EXPT_NAME="test-evolve-barriers"
EXPT_DIR="${PB_BASE_DIR}/m3/expts/${EXPT_NAME}"
CONFIG_FILE="${EXPT_DIR}/${EXPT_NAME}.cfg"

GCC_MODULE="linux-rocky9-cascadelake/gcc/15.2.0-gcc-11.5.0"

THIS_JOB_ID="${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}"

RUN_CMD="${PB_EXEC} -c ${CONFIG_FILE} --log-filename-prefix ${EXPT_NAME}-${THIS_JOB_ID}"

#############################################################

echo -n "*** Job ${THIS_JOB_ID} started at "; date

module load ${GCC_MODULE}
echo "Loaded module ${GCC_MODULE}"

cd ${PB_BASE_DIR}

echo "Running command: ${RUN_CMD}"
${RUN_CMD}

echo "Moving output files to ${EXPT_DIR}"
mv ${PB_BASE_DIR}/output/${EXPT_NAME}-${SLURM_ARRAY_JOB_ID}* ${EXPT_DIR}

echo -n "*** Job ${THIS_JOB_ID} ended at "; date

#############################################################
