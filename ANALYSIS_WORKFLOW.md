# How to perform analysis and produce graphs of polybee experiments

## Analysis of a single condition across multiple runs

### Production of fitness CSV files:

Produces a csv file in the format: ISLAND,GEN,FITNESS

```
EXPT_BASENAME="evolve-bridge-and-barrier-tests"
NUM_REPS=50
RUN_TITLE="Evolve positions of 20 barriers and 10 bridges"

for N in `seq 1 $NUM_REPS`; do gawk '/INFO: isl/ {print $3","$5","$11}' out-${EXPT_BASENAME}-*_$N.txt > fitness-${EXPT_BASENAME}_$N.csv; done

~/polybee/tools/gen_champion_fitness_csv.py --basename ${EXPT_BASENAME} out-${EXPT_BASENAME}-*.txt

for N in `seq 1 $NUM_REPS`; do ~/polybee/tools/plot_fitness.py --type 1 --one-island --ymin -0.78 --ymax -0.50 --minimal --save-only --title "${RUN_TITLE} (Run ${N})" --basename ${EXPT_BASENAME} fitness-${EXPT_BASENAME}_$N.csv; done
```
