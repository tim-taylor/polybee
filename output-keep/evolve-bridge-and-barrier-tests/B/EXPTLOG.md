# Purpose

To plot results of an evolutionary run using the `evolve-bridge-and-barrier-tests-B.cfg` config
file, which does an initial evolutionary test of evolving bridge positions.
It uses flower-min/max-visit-count-success as the objective function rather than EMD distance.

The evolutionary run uses 3 islands each running the sga algorithm, with migration period 20.

# Code

Run the experiment with the following command:

```
./run-release -c config-files/evolve-bridge-and-barrier-tests-B.cfg | tee out-evolve-bridge-and-barrier-tests-B.txt
```

From the output of the above, make a CSV file containing island number, generation number and fitness score for each evaluation:

```
gawk '/INFO: isle/ {print $3","$5","$11}' out-evolve-bridge-and-barrier-tests-B.txt > out-evolve-bridge-and-barrier-tests-B.csv
```

Now plot this data:

```
../../tools/plot_fitness_islands.py --type 1 out-evolve-bridge-and-barrier-tests-B.csv
```

and then save the plot via the GUI of the `plot_fitness_islands.py` script, and rename it:

```
mv ~/Figure_1.png out-evolve-bridge-and-barrier-tests-B.png
```
