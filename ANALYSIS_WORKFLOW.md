# How to perform analysis and produce graphs of polybee experiments

## Analysis of a single condition across multiple runs

### Production of fitness CSV files:

Produces a csv file in the format: ISLAND,GEN,FITNESS

```
RAW_OUTPUT_DIR="raw-output"
EXPT_BASENAME="evolve-20X-10B-400gen-400pop-100epi-2000its"
NUM_REPS=50
RUN_TITLE="Evolve positions of 20 barriers and 10 bridges"
FLOWMAP_DIR="flowmap-runs"
FITNESS_PLOT_WORST="-0.50"
FITNESS_PLOT_BEST="-0.78"
FLOWMAP_COUNT_TH="0.1"
FLOWMAP_STRENGTH_TH="0.5"
HEATMAP_CELL_SIZE=25
FLOWMAP_CELL_SIZE=50

for N in `seq 1 $NUM_REPS`; do gawk '/INFO: isl/ {print $3","$5","$11}' ${RAW_OUTPUT_DIR}/out-${EXPT_BASENAME}-*_$N.txt > fitness-${EXPT_BASENAME}_$N.csv; done

~/polybee/tools/gen_champion_fitness_csv.py --basename ${EXPT_BASENAME} ${RAW_OUTPUT_DIR}/out-${EXPT_BASENAME}-*.txt

for N in `seq 1 $NUM_REPS`; do ~/polybee/tools/plot_fitness.py --type 1 --one-island --ymin $FITNESS_PLOT_BEST --ymax $FITNESS_PLOT_WORST --minimal --save-only --title "${RUN_TITLE} (Run ${N})" --basename ${EXPT_BASENAME} fitness-${EXPT_BASENAME}_$N.csv; done

~/polybee/tools/plot_fitness.py --type 1 --one-island --ymin $FITNESS_PLOT_BEST --ymax $FITNESS_PLOT_WORST --minimal --save-only --title "${RUN_TITLE}" --basename ${EXPT_BASENAME} fitness-${EXPT_BASENAME}_*.csv

~/polybee/tools/plot_fitness.py --type 1 --minimal --save-only --title "${RUN_TITLE} (All Runs)" --basename ${EXPT_BASENAME} fitness-${EXPT_BASENAME}_*.csv

for N in `seq 1 $NUM_REPS`; do ~/polybee/tools/best_individual_to_cfg.py ${RAW_OUTPUT_DIR}/out-${EXPT_BASENAME}-*_$N.txt; done
```

### Barrier and Bridge Heatmaps and Barrier Flowmaps

```
~/polybee/tools/gen_bx_heatmaps.py --cell-size $HEATMAP_CELL_SIZE --basename heatmap-${EXPT_BASENAME} best-${EXPT_BASENAME}-*.cfg

~/polybee/tools/visualize_heatmap.py --title "Barrier position heatmap: ${RUN_TITLE} (${NUM_REPS} runs)" --config best-${EXPT_BASENAME}-*_1.cfg --save-only heatmap-${EXPT_BASENAME}_barriers.csv

~/polybee/tools/visualize_heatmap.py --title "Bridge position heatmap: ${RUN_TITLE} (${NUM_REPS} runs)" --config best-${EXPT_BASENAME}-*_1.cfg --save-only heatmap-${EXPT_BASENAME}_bridges.csv

~/polybee/tools/gen_barrier_flowmap.py --cell-size $FLOWMAP_CELL_SIZE --basename "barrier-flowmap-size-${FLOWMAP_CELL_SIZE}-${EXPT_BASENAME}" best-${EXPT_BASENAME}-*.cfg

~/polybee/tools/visualize_flowmap.py --color -c best-${EXPT_BASENAME}-*_1.cfg -t "Summary of evolved barrier positions" --save-only flowmap-${EXPT_BASENAME}-barrier-flowmap.csv

~/polybee/tools/visualize_flowmap.py --color --count-th $FLOWMAP_COUNT_TH --strength-th $FLOWMAP_STRENGTH_TH -c best-${EXPT_BASENAME}-*_1.cfg -t "Summary of evolved barrier positions (thresholds: count=${FLOWMAP_COUNT_TH}, strength=${FLOWMAP_STRENGTH_TH})" --save-only flowmap-${EXPT_BASENAME}-barrier-flowmap.csv
```

###  Bee movement flowmaps for a given experimental condition - individual runs and merged across replicates

```
if [ ! -d "$FLOWMAP_DIR-$FLOWMAP_CELL_SIZE" ]; then
    mkdir "$FLOWMAP_DIR-$FLOWMAP_CELL_SIZE"
fi
cd "$FLOWMAP_DIR-$FLOWMAP_CELL_SIZE"
for N in `seq 1 $NUM_REPS`; do ~/polybee/run -c ../best-${EXPT_BASENAME}-*_$N.cfg --flowmap-cell-size $FLOWMAP_CELL_SIZE --target-heatmap-filename ~/polybee/target-heatmaps/target-heatmap-13x16-4-rows.csv --visualise false --log-dir .; done
cd ..

~/polybee/tools/merge_flowmaps.py bee-flowmap-size-${FLOWMAP_CELL_SIZE}-intra-condition-merged-${EXPT_BASENAME}.csv $FLOWMAP_DIR-$FLOWMAP_CELL_SIZE/${EXPT_BASENAME}-*-flowmap*csv

~/polybee/tools/visualize_flowmap.py --color -c best-${EXPT_BASENAME}-*_1.cfg -t "Bee flowmap: ${RUN_TITLE}" --save-only bee-flowmap-size-${FLOWMAP_CELL_SIZE}-intra-condition-merged-${EXPT_BASENAME}.csv

~/polybee/tools/visualize_flowmap.py --color --count-th $FLOWMAP_COUNT_TH --strength-th $FLOWMAP_STRENGTH_TH -c best-${EXPT_BASENAME}-*_1.cfg -t "Bee flowmap: ${RUN_TITLE} (thresholds: count=${FLOWMAP_COUNT_TH}, strength=${FLOWMAP_STRENGTH_TH})" --save-only bee-flowmap-size-${FLOWMAP_CELL_SIZE}-intra-condition-merged-${EXPT_BASENAME}.csv
```



# NOTES

Testing this stuff with data in:

```
/home/tim/tmp/polybee-data/evolve-20X-10B-400gen-400pop-100epi-2000its
```
