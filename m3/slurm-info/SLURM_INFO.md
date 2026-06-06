# SLURM INFO

SLURM files control the batched execution of experiments on M3

## SLURM FILE contents

The basic layout of a SLURM file is as follows:

```
#!/bin/bash
#SBATCH --job-name="Example job name!"
#SBATCH --time=1:00:00 # specify a max time limit the job might need. Job will be killed if it exceeds this!
#SBATCH --cpus-per-task=1
#SBATCH --mem=1G

echo "This job has started!"
# Sleep to pretend we're doing some useful work...
sleep 10
echo "This job has ended!"
```

### Details of individual SLURM commands

**time**

Format:

`day-hours:minutes:seconds`

Examples:

* `--time=15`         # ask for 15 mins
* `--time=1:30:00`    # ask for 1 hr 30 mins
* `--time=2-12:00:00` # ask for 2 days, 12 hrs

**memory**

Slurm offers multiple options for configuring memory. The only ones you'll need are:

`--mem: memory per node.`

`--mem-per-cpu: memory per CPU.`

If your job only uses one node, then use --mem to specify the total memory. E.g. to ask for 64 GB of memory (per node):

`--mem=64G`

**number of CPUs**

_note_

Slurm distinguishes between CPUs, cores, and sockets. You are probably best off only ever thinking in terms of CPUs.

If you just want 8 CPUs for a particular job:

--cpus-per-task=8

Note you should only ask for multiple CPUs if the program you are running will actually use them! Some programs do not have any multithreading or multiprocessing and so cannot use more than one CPU at a time.

**array jobs**

It is possible to submit array jobs, that are useful for automating the running of parametric tasks. The command:

`#SBATCH --array=1-300`

specifies that 300 jobs are submitted to the queue, and each one has a unique identifier specified in the environment variable `SLURM_ARRAY_TASK_ID` (in this case ranging from 1 to 300). This is in addition to the `SLURM_ARRAY_JOB_ID` of the job.

So the JOB_ID of an array job will look like

`${SLURM_ARRAY_JOB_ID}_${SLURM_ARRAY_TASK_ID}`

for use in `squeue`, `scontrol` etc.

Here is an example script:

```
#!/bin/env bash
#SBATCH --job-name=sample_array
#SBATCH --time=10:00:00
#SBATCH --mem=4000
# make 300 different jobs from this one script!
#SBATCH --array=1-300
#SBATCH --output=job.out

module load modulefile
myExe.exe ${SLURM_ARRAY_TASK_ID} # equivalent to SGE's ${SGE_TASK_ID}
```

**project charging and accounting**

Each project is assigned a share on the resources on the system. To charge usage towards your project, please set the project ID in the account entry of your job scripts as follows:

`#SBATCH --account=tf31`

**getting emails**

You can ask for an email to be sent at different stages of your job's lifecycle. Set --mail-user to your email address:

`--mail-user=my-email@monash.edu`

You also need to specify which events should trigger an email. See the --mail-type documentation for all the options, but for simplicity, you can just ask for emails for all events with:

`--mail-type=ALL`

Available options are: NONE, BEGIN, END, FAIL, REQUEUE, ALL, INVALID_DEPEND, STAGE_OUT, TIME_LIMIT, TIME_LIMIT_50, TIME_LIMIT_80, TIME_LIMIT_90, ARRAY_TASKS.

Multiple type values may be specified in a comma separated list.

For full info on these, see https://slurm.schedmd.com/sbatch.html

**changing output files**

By default, Slurm saves all of the output of your job (that would ordinarily be printed to the terminal) to a single file called:

`slurm-<JOBID>.out`

where <JOBID> is the number corresponding to your job's ID. This file will be placed in whichever directory you ran sbatch from. You can change this by specifying `--output`. You can optionally redirect all error output using `--error`.

## DIRECTORIES AND WHERE TO STORE DATA

Within your `$HOME` directory there are links to the project′s primary and scratch folders on M3s parallel file system:

* `/home/ttay0006/tf31`
* `/home/ttay0006/tf31_scratch`
* `/home/ttay0006/tf31_scratch2`

Please store your **primary data** under the `tf31` and **reproducible data** under the `scratch` or `scratch2` spaces.

The `user_info` command may be used to view the current usage and quota of these folders.

For details, see: https://docs.erc.monash.edu/M3/Files/KeyDirectories

## EXECUTION AND OTHER COMMANDS

**To run a slurm file:**

`sbatch my-script.slurm`

**To view a summary of currently queued and running jobs:**

`show_job`

or

`squeue --me` # for more control of what is displayed

**To cancel/kill a jobs:**

`scancel <JOBID>`

**To view a summary of past jobs, how long they ran for and max memory used, etc:**

`mon_sacct`

To get a report of your CPU and GPU usage, you can also run:

`jobstats JOB_ID`

## SOURCES OF FURTHER INFORMATION

* https://slurm.schedmd.com/quickstart.html
* https://docs.erc.monash.edu/Compute/HPC/M3/RunningJobsOnM3/
* Example SLURM files in my home dir on M3 under `~/example-slurm-scripts`
