# Purpose

To plot results of an evolutionary run with island model enabled, such as that run by the `do-island-test-4b` script

3 islands running 3 algs (sga, pso_gen, gaco), with
migration period 1


# Code

Let's assume we've run the `do-island-test-4b` script, which looks like this:

```
./run-release -c config-files/evolve-entrance-positions-4-rows-with-energetics.cfg --num-bees 100 --num-trials-per-config 150 --num-configs-per-gen 50 --num-generations=50 --num-islands 3 --migration-period 1 --migration-num-select 1 --migration-num-replace 1 | tee out-island-test.txt
```

From the output of the above, make a CSV file containing generation number and EMD score for each evaluation:

```
gawk '/INFO: isle/ {print $3","$5","$16}' out-island-test.txt > out-island-test.csv
```

Now plot this data:

```
../../tools/plot_emd_scores_islands.py out-island-test.csv
```

and then save the plot via the GUI of the `plot_emd_scores.py` script, and rename it:

```
mv ~/Figure_1.png out-island-test.png
```
