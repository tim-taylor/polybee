---
title: "PolyBee: An Agent-Based Model of Bee Movement in Polytunnels"
author:
  - Tim Taylor
date: \today
abstract: |
  One-paragraph summary of motivation, approach, and headline findings.
  Fill this in last, once the results section is written.
bibliography: references.bib
link-citations: true
numbersections: true
toc: true
figPrefix: "Figure"
eqnPrefix: "Equation"
tblPrefix: "Table"
secPrefix: "Section"
linkReferences: true
nameInLink: true
---

# Introduction

## Background and motivation

Why polytunnels, why bee movement, why an agent-based model. What
question does PolyBee answer that field observation or prior models
cannot.

## Related work

Prior models / studies this builds on or departs from [@placeholder2024].

# System design

## Overview

High-level description of the simulation: agents, environment,
time-stepping.

## Bee agent model

Movement rule, e.g. bounded turning per step:

$$
\theta_{t+1} = \theta_t + \Delta\theta, \qquad
|\Delta\theta| \le \delta_{\max}
$$

where $\delta_{\max}$ is `bee-max-dir-delta`.

## Environment and hives

Boundaries, collision handling, hive placement and orientation.

## Parameters

Reference table of key parameters (pull from `polybee.cfg` /
`PARAM-NOTES.md`).

| Parameter            | Meaning                     | Typical value |
|----------------------|------------------------------|---------------|
| `env-width/height`   | Environment extent (units)  | 800           |
| `bee-max-dir-delta`  | Max heading change per step  | 0.5           |
| `num-iterations`     | Simulation steps             | 2000          |

: Key simulation parameters {#tbl:params}

See @tbl:params for the full parameter set used in the experiments below.

# Experiments

## Design

What was varied (e.g. cell size, delta color scale, number of bees),
what was held constant, and why.

## Setup

Reference to `run_analysis.sh` / `run_cross_analysis.sh` invocations,
config files used, number of replicates.

# Results and analysis

## Result 1

![Caption describing what the figure shows.](figures/placeholder.png){#fig:placeholder width=80%}

As shown in @fig:placeholder, ...

## Discussion

Interpretation, limitations, surprises.

# Conclusion

Summary of findings and future work.

# References
