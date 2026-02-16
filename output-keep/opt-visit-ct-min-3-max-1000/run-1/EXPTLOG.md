# Purpose

To plot results of an evolutionary run launched by the `do-opt-visit-ct-min-3-max-1000` script, which
does an initial evolutionary test of using flower-min/max-visit-count-success as the objective
function rather than EMD distance.

The evolutionary run uses 3 islands running 3 algs (sga, pso_gen, gaco), with
migration period 10 (as was used in the previous `do-island-test-4` run.

# Code

The `do-opt-visit-ct-min-3-max-1000` script runs the following command:

```
./run-release -c config-files/evolve-entrance-positions-4-rows-with-energetics-2.cfg --num-bees 50 --num-trials-per-config 150 --num-configs-per-gen 50 --num-generations=50 --num-islands 3 --migration-period 10 --migration-num-select 1 --migration-num-replace 1 --use-diverse-algorithms true --evolve-objective 1 --min-visit-count-success 3 --max-visit-count-success 1000 | tee out-opt-visit-ct-min-3-max-1000.txt
```

From the output of the above, make a CSV file containing generation number and EMD score for each evaluation:

```
gawk '/INFO: isle/ {print $3","$5","$16}' out-opt-visit-ct-min-3-max-1000.txt > out-opt-visit-ct-min-3-max-1000.csv
```

Now plot this data:

```
../../tools/plot_fitness_islands.py --type 1 out-opt-visit-ct-min-3-max-1000.csv
```

and then save the plot via the GUI of the `plot_fitness_islands.py` script, and rename it:

```
mv ~/Figure_1.png out-opt-visit-ct-min-3-max-1000.png
```
