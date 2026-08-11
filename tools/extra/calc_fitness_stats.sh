#!/bin/bash
for C in baseline-runs-2000its \
    evolve-10B-400gen-400pop-100epi-2000its \
    evolve-20X-400gen-400pop-100epi-2000its \
    evolve-20X-10B-400gen-400pop-100epi-2000its; do
  echo $C
  datamash -t, min 2 median 2 q1 2 q3 2 < $C/fitness-data-agg/champion-fitnesses-$C.csv
  echo
done
