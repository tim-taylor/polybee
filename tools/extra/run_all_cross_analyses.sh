#!/bin/bash
~/polybee/tools/run_cross_analysis.sh --delta-color-scale-max 3.10 --title "Evolve positions for 20 barriers and 10 bridges vs Baseline" evolve-20X-10B-400gen-400pop-100epi-2000its baseline-runs-2000its
~/polybee/tools/run_cross_analysis.sh --delta-color-scale-max 3.10 --title "Evolve positions for 20 barriers vs Baseline" evolve-20X-400gen-400pop-100epi-2000its baseline-runs-2000its
~/polybee/tools/run_cross_analysis.sh --delta-color-scale-max 3.10 --title "Evolve positions for 10 bridges vs Baseline" evolve-10B-400gen-400pop-100epi-2000its baseline-runs-2000its
~/polybee/tools/run_cross_analysis.sh --delta-color-scale-max 3.10 --title "Evolve positions for 20 barriers and 10 bridges vs 20 barriers only" evolve-20X-10B-400gen-400pop-100epi-2000its evolve-20X-400gen-400pop-100epi-2000its/
~/polybee/tools/run_cross_analysis.sh --delta-color-scale-max 3.10 --title "Evolve positions for 20 barriers and 10 bridges vs 10 bridges only" evolve-20X-10B-400gen-400pop-100epi-2000its evolve-10B-400gen-400pop-100epi-2000its
