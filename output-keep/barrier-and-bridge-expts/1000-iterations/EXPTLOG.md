# Barrier and bridge expts

These experiments look at the affect of intra-row barriers and between-row bridges in the
tunnel on the resulting Successful Visit fraction.

## Run expts

Each configuration is run 100 times. The following line performs all experiments for 4 different configurations:

```
for E in base BAR BRG BAR-BRG; do for N in {1..100}; do ./run-release -c config-files/barrier-and-bridge-expts-$E.cfg --visualise false; done; done
```

## Preparation of data files

All output files apart from the run-info files can be discarded. Save the 100 run-info files from each config in a separate directory.

For each directory of 100 results, generate a file containing just the Successful Visit fraction for each run, as follows:

```
for F in `ls barrier-and-bridge-expts-*`; do gawk '/Successful visit/ {print $6}' $F >> visit-fractions-barrier-and-bridge-expts-[CONFIGID].csv; done
```

## Plot stats

Generate a box and whiskers plot showing all configs, as follows:

```
../../tools/plot_boxplot.py visit-fractions-barrier-and-bridge-expts-base.csv visit-fractions-barrier-and-bridge-expts-BRG.csv visit-fractions-barrier-and-bridge-expts-BAR.csv visit-fractions-barrier-and-bridge-expts-BAR-BRG.csv --title "Barriers, Bridges and Successful Visit fraction" --ylabel "Successful Visit fraction" --labels "Base Config" "Bridges" "Barriers" "Barriers & Bridges" --ymin 0.625 --ymax 1.025 | tee stats-visit-fractions-barrier-and-bridge-expts.txt
```
