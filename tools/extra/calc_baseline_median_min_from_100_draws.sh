#!/bin/bash
for N in {1..100}; do
    shuf baseline-runs-2000its/fitness-data-agg/champion-fitnesses-baseline-runs-2000its.csv | head -50 > tmp-50.csv
    datamash -t, min 2 < tmp-50.csv
    rm -f tmp-50.csv
done > tmp-50-min-100reps.csv
echo "The median min found in 100 random selections of 50 samples from baseline results is:"
datamash -t, median 1 < tmp-50-min-100reps.csv
rm -f tmp-50-min-100reps.csv
