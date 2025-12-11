# Purpose

To plot results of an evolutionary run such as that run by the `do-evol` and `do-evol-with-energetics` scripts

# Code

Let's assume we've run the do-evol-with-energtics script, which looks like this:

```
./run-release -c config-files/evolve-entrance-positions-4-rows-with-energetics.cfg --num-bees 100 --num-trials-per-config 150 --num-generations=50 | tee out-sga-with-energetics-100-bees-150-reps-50-gens.txt
```

From the output of the above, make a CSV file containing generation number and EMD score for each evaluation:

```
gawk '/INFO: Gen/ {print $3","$14}' out-sga-with-energetics-100-bees-150-reps-50-gens.txt > out-sga-with-energetics-100-bees-150-reps-50-gens.csv
```

Now plot this data:

```
./tools/plot_emd_scores.py out-sga-with-energetics-100-bees-150-reps-50-gens.csv
```

and then save the plot via the GUI of the `plot_emd_scores.py` script, and rename it:

```
mv ~/Figure_1.png out-sga-with-energetics-100-bees-150-reps-50-gens.png
```
