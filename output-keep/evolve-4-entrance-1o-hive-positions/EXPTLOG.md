# Purpose

To plot results of an evolutionary run launched by the `do-evolve-4-entrance-1o-hive-positions` script,
which does an initial evolutionary test of evolving both entrance and hive (outside) positions.
It uses flower-min/max-visit-count-success as the objective function rather than EMD distance.

The evolutionary run uses 3 islands running 3 algs (sga, pso_gen, gaco), with
migration period 10 (as was used in the previous `do-island-test-4` run.

# Code

The `do-evolve-4-entrance-1o-hive-positions` script runs the following command:

```
./run-release -c config-files/evolve-4-entrance-1o-hive-positions.cfg | tee out-evolve-4-entrance-1o-hive-positions.txt
```

From the output of the above, make a CSV file containing island number, generation number and fitness score for each evaluation:

```
gawk '/INFO: isle/ {print $3","$5","$11}' out-evolve-4-entrance-1o-hive-positions.txt > out-evolve-4-entrance-1o-hive-positions.csv
```

Now plot this data:

```
../../tools/plot_fitness_islands.py --type 1 out-evolve-4-entrance-1o-hive-positions.csv
```

and then save the plot via the GUI of the `plot_fitness_islands.py` script, and rename it:

```
mv ~/Figure_1.png out-evolve-4-entrance-1o-hive-positions.png
```
