# Title:
param-tuning-tests-with-energetics

# Purpose:
The purpose of these experiments is to test the variability of simulation results when run on the same environmental config with 4 specified entrances. We test variability for several different numbers of bees, and for several different numbers of repeats. These expts are run with the newly created bee energetics mechanisms.

# Commands to run tests:

To run 100 reps each of 25, 50, 75, 100 bees:

```
for N in 25 50 75 100; do R=100; for T in $(seq 1 $R); do ./run-release -c config-files/param-config-tests-with-energetics.cfg --num-bees $N --log-filename-prefix=param-config-tests-with-energetics-$N-bees-$R-reps-run-$T; done; done
```

To create aggregated results files for each N,R combo:

```
for N in 25 50 75 100; do R=100; gawk '/Final EMD/ {print $9}' param-config-tests-with-energetics-$N-bees-$R-reps-*.txt > param-config-tests-with-energetics-$N-bees-$R-reps-all.csv ; done
```

To produce box and whisker plots for the aggregated results files:

```
for N in 25 50 75 100; do R=100; ~/projects/polybee/tools/plot_boxplot.py --save-only param-config-tests-with-energetics-$N-bees-$R-reps-all.csv; done
```

And similarly, to test different numbers of reps for 100 bees:

```
for R in 10 20 30 50 100 150; do N=100; for T in $(seq 1 $R); do ./run-release -c config-files/param-config-tests-with-energetics.cfg --num-bees $N --log-filename-prefix=param-config-tests-with-energetics-$N-bees-$R-reps-run-$T; done; done

for R in 10 20 30 50 100 150; do N=100; gawk '/Final EMD/ {print $9}' param-config-tests-with-energetics-$N-bees-$R-reps-*.txt > param-config-tests-with-energetics-$N-bees-$R-reps-all.csv ; done

for R in 10 20 30 50 100 150; do N=100; ~/projects/polybee/tools/plot_boxplot.py --save-only param-config-tests-with-energetics-$N-bees-$R-reps-all.csv; done
```
