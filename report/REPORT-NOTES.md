## Rough notes for completing results and analysis section

Main figures:
(A) Baseline flow field - baseline/bee-flowmaps-agg [cell size 10,25,50, th N,Y]
(B) Flow field with evolved barriers/bridges - evolve/bee-flowmaps-agg
(C) Axial angular-difference heatmap - cross-analysis-evolve-vs-baseline/size-S-bin-B-[no]thresh-angdelta-heatmap [size 10,25,50, bin 5,10,15] => use size 50, bin 5
(D) Weighted histogram of delta-theta_axial - cross-analysis-evolve-vs-baseline/size-S-bin-B-[no]thresh-angdelta-histogram [size 10,25,50, bin 5,10,15] => use size 50, bin 5

And then a second set of figures showing functional effect:
(E) Pollination/visitation difference map - cross-analysis-evolve-vs-baseline/bee-heatmap-delta [cell size 10,25,50] => size 50

For each type of graph, ask Claude to look at the script and look at the command used to generate the graph, and produce a brief description of what exactly is being plotted (inc. are we weighting by sample size, etc)

The strongest version is to show that large angular changes occur in biologically meaningful places, and that those places correspond to improved visitation or pollination.

Important details

* Weight or mask by sample size. Otherwise cells with very few bees can show dramatic but meaningless angle changes.

* Use axial difference only where direction is genuinely orientation-like. If forward vs backward movement matters biologically, also show ordinary directional angular difference.

* Compare angle change to outcome change. A high angular change is not automatically good; the best evidence is:

  - barrier placement → local flow redirection → changed visitation pattern → improved pollination

So: yes, show the delta map, but treat it as only one panel in a richer analysis.
