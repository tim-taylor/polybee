# Patch arrangement expts

These experiments look at the affect of different arrangements of plant patches in the
tunnel on the resulting Successful Visit fraction.

## Run expts

Each configuration is run 100 times. The following line performs all experiments for 4 different configurations:

```
for E in base HC VC VHC; do for N in {1..100}; do ./run-release -c config-files/patch-arrangement-expts-$E.cfg --visualise false; done; done
```

## Preparation of data files

All output files apart from the run-info files can be discarded. Save the 100 run-info files from each config in a separate directory.

For each directory of 100 results, generate a file containing just the Successful Visit fraction for each run, as follows:

```
for F in `ls patch-arrangement-expts-*`; do gawk '/Successful visit/ {print $6}' $F >> visit-fractions-patch-arrangement-expts-[CONFIGID].csv; done
```

## Plot stats

Generate a box and whiskers plot showing all configs, as follows:

```
../../tools/plot_boxplot.py visit-fractions-patch-arrangement-expts-*.csv --ylabel "Successful Visit fraction" --labels "Base Config" "Horizontal Compact" "Vertical Compact" "Vertical & Horizontal Compact" --title "Patch arrangement and Successful Visit fraction" --ymin 0.625 --ymax 1.025| tee stats-visit-fractions-patch-arrangement-expts.txt
```
