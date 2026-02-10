# Purpose

To test various configurations of tunnel entrance and net on what coverage is achieved (as measured by EMD score at the end of a simulation.

Four configurations are tested:

* (a) whole end open
* (b) whole end open with anti-hail netting
* (c) evolved entrance positions (from the island-model-test-4 results)
* (d) evolved entrance positions with anti-hail-netting

# Code

100 replicates of each configuration are run:

```
for E in 1a 1b 1c 1d; do for N in {0..99}; do ./run-release -c config-files/net-expt-$E.cfg; done; done
```

For each configuration, create a file that just lists the 100 EMD scores obtained:

```
for E in 1a 1b 1c 1d; do for F in `ls net-expt-$E-run-info*.txt`; do tail -1 $F | gawk '{print $9}' >> net-expt-$E-emd-values.txt; done; done
```

Now calculate the mean, median and std dev of each config, and calculate whether the differences are significant:

```
../../tools/stats.py -g --ylabel 'EMD to uniform over patches' --title 'Median EMD values per configuration' net-expt-1*-emd-values.txt
```
