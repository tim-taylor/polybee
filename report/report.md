---
title: "PolyBee: An Agent-Based Model of Bee Movement in Polytunnels"
author:
  - Tim Taylor
  - Alan Dorin
date: \today
abstract: |
  PolyBee is an agent-based model of bee movement and visitation of crops in polytunnels.
  It also includes an optimisation component that allows searching for good arrangements
  of baffles/barriers and/or "bridge" pots in tunnel to promote homogeneous visitation
  patterns by the bees of all crops in the tunnel. This report describes the background
  and motivation behind the development of the model, followed by a full description of
  the model itself. It then describes a series of experiments on the optimisation of
  barrier and bridge placement. Results suggest that appropriately-placed barriers can
  significantly promote homogeneous plant visitation (thereby leading to improved
  levels of pollination). Bridge placement, on the other hand, was much less effective
  in improving pollination.
documentclass: article
classoption:
  - 11pt
  - twoside
header-includes: |
  \input{preamble.tex}
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

This report describes the `polybee` simulation, an agent-based model of
bee pollination of crops planted in a polytunnel. The aim of
the project is to search for physical interventions in the design
and layout of a given polytunnel (such as the introduction
of barriers/baffles or bridging pots) that can lead to a better spatial coverage
of crops by the bees, and thereby result in improved crop production.

On top of the agent-based model aspect of `polybee`, the software also
includes a flexible evolutionary optimisation system. This allows the
system to search for good arrangements of barriers and/or bridges to
maximise pollination in a given polytunnel.

The `polybee` code is open-source and available at
[https://github.com/tim-taylor/polybee](https://github.com/tim-taylor/polybee).
System documentation is available at
[https://tim-taylor.github.io/polybee/](https://tim-taylor.github.io/polybee/).

In general terms, the philosophy behind the `polybee` optimisation approach is
to calibrate the agent-based model by *environment shaping*, that is, by tuning
the configuration of the agents' physical environment. This is in contrast to
the more traditional approach of calibration by tuning the behavioural
parameters of the agents, and is beneficial in scenarios where it is not
possible to change the agents' behaviour directly.

In the following section we provide some introductory comments on the environment
shaping approach in general. We then present details of the `polybee` system
design, followed by a description of initial optimisation experiments and
an analysis of results obtained.

## Environment shaping in ABMs {#sec:environment-shaping-in-abms}

Agent-based modelling (ABM) is widely used to study complex systems in which
system-level outcomes emerge from the local interactions of heterogeneous,
situated agents. In ecology and related fields, ABMs are particularly valuable
when spatial structure, local interactions, and behavioural heterogeneity are
central to the phenomena of interest [@grimm2005; @railsback2019; @an2021].
Because agents in such models interact both with one another and with a
spatially structured environment, ABMs are natural tools for studying the
emergence of non-trivial spatial distributions.

A recurring goal in applied ABM is to obtain model outcomes that match empirical
observations or desired macroscopic patterns. Much of the methodological
literature addresses this through calibration: behavioural rules or their
parameters are adjusted until the model reproduces observed aggregate outcomes
[@platt2020; @lamperti2018; @hazelbag2020]. In ecology, pattern-oriented
modelling has been especially influential in this regard, treating multiple
observed patterns as constraints that help identify plausible underlying
mechanisms [@grimm2012]. Even where the focal patterns are spatial, however, the
main levers of adjustment are commonly the behavioural or decision-making rules
of agents [@baey2023; @vandervaart2015].

That emphasis is understandable, but it leaves underdeveloped a different class
of intervention problem. In many real systems, the agents' behaviour is not
easily or directly modifiable. In many cases, agents may respond according to
fixed or only weakly malleable behavioural repertoires (see below for further
discussion of this point). By contrast, the structure of the environment in
which they move and interact may be open to design or intervention. Habitat
patches can be added, removed, or rearranged; corridors can be opened or closed;
resources can be redistributed; barriers can be introduced; and the topology of
movement opportunities can be reshaped. In such cases, the practical question is
not how to alter the agents, but how to alter the environment so that the
unchanged agents collectively generate a desired spatial outcome.

This paper develops an approach to that problem. We assume that a desired,
possibly strongly non-homogeneous, distribution of agents is given. We further
assume that agent behaviour is fixed. The task is therefore an inverse one: to
identify environmental configurations that induce the target distribution, or
approximate it as closely as possible, through the interaction of agents with
their surroundings. This reframes a familiar optimisation problem in ABM.
Instead of fitting agent parameters to reproduce patterns, we treat
environmental configuration as the primary decision variable and the emergent
agent distribution as the optimisation target.

Our framing is well motivated by ecological theory. Landscape ecology has long
emphasised that ecological processes depend not only on local habitat quality
but also on spatial pattern, including habitat amount, configuration,
fragmentation, and connectivity [@turner1989; @turner2005; @fahrig2003].
Metapopulation theory likewise shows that occupancy and persistence depend
critically on the arrangement of habitat patches and the movement opportunities
they create [@hanski1999]. Movement ecology, in turn, treats movement as arising
from the interaction of an organism's internal state, motion capacity,
navigation capacity, and external environmental factors [@nathan2008]. Together,
these literatures imply that large-scale spatial distributions are often highly
sensitive to environmental structure even when individual behavioural rules are
unchanged.

Related ideas also appear in planning and design-oriented uses of ABM. Some
studies use ABMs to compare alternative landscapes, land-use plans, or
built-environment layouts, asking how different spatial configurations affect
emergent outcomes. These studies move toward an environment-centred perspective,
but typically they evaluate a limited set of candidate scenarios, or align ABMs
with external optimisation over land-use patterns (e.g. [@voloshin2015]).
[@orsi2019] presents an interesting case where agents representing households
are modelled to investigate desirable urban planning layouts, but in this case
it is the agents themselves that form the environment for other agents, and the
evaluation criteria measure how well an emergent spatial distribution of
households satisfies those households' desires. Similarly, [@bartkowski2020]
describe an ABM in which agents representing farmers are modelled to investigate
sustainable land-use patterns. Here again, the patterns that emerge from the
farmer agents are evaluated according to economic service models: the work does
not seek to achieve a particular distribution of agents as such. These studies
stop short of formulating the general inverse problem that concerns us here:
given a target agent distribution, how should the environment be configured so
as to produce it?

A further conceptual motivation comes from the literature on self-organisation
and the control of complex systems. In many distributed systems, desired global
behaviour cannot be imposed straightforwardly by centralised command or by
rewriting local rules in a fine-grained way. Instead, the system may be guided
indirectly by shaping constraints, interaction opportunities, or informational
structure [@gershenson2003; @gershenson2020; @prokopenko2014]. Similarly, some
authors in Artificial Life have argued for a perspective on the open-ended
evolution of biological function that emphasizes the role of genes as evolved
constraints upon chemical reactions and physical phenomena [@pattee1995;
@taylor2004; @taylor2021]. Although much of that literature is not specifically
ecological, it provides an important conceptual precedent: complex collective
dynamics can often be steered through the design of the conditions under which
local interactions occur. Our approach can be understood as an application of
this broader idea to spatial ABMs, with environmental configuration functioning
as the medium of indirect control.

The contribution of the present paper is therefore both methodological and
conceptual. Methodologically, we propose a framework for searching over
environmental configurations in order to achieve specified emergent spatial
distributions. Conceptually, we shift attention from the usual question of how
to calibrate agents to the complementary question of how to design environments.
For ecological, socio-ecological, and other spatially embedded systems in which
direct behavioural intervention is infeasible, this change of perspective opens
up a distinct and potentially powerful mode of analysis and design.

The problem formulated here belongs to a class in which the objective function
is available only through simulation, is stochastic, and has no accessible
gradient. This places it firmly in the domain of black-box optimisation, for
which evolutionary and metaheuristic methods have been extensively developed
[@eiben2015]. Evolutionary algorithms, in particular, have a well-established
record in optimising ABM parameters: genetic algorithms and evolution strategies
have been applied to agent rule calibration, parameter estimation, and scenario
search across a range of ecological and social models [@stonedahl2010;
@moya2021; @robles2021]. That literature, however, is again almost exclusively
directed at behavioural or parametric optimisation of agents rather than at the
configuration of the environment. The present work inherits the same class of
search machinery but redirects it toward a different decision variable, treating
the spatial environmental structure as the entity to be evolved.

# System design {#sec:system-design}

An overview of the `polybee` simulation is presented below, following a concise
form of the ODD (Overview, Design concepts, Details) protocol for describing
agent-based models [@grimm2020odd]. This description covers only the simulation
itself -- bee movement, the polytunnel environment, foraging, and observation --
and not the separate evolutionary optimisation layer (`PolyBeeEvolve`) that
searches over environment configurations using the simulation as its fitness
evaluator (described in the Experiments section). A complete, submodel-level ODD
description, sufficient to support reimplementation, is given in
@sec:appendix-full-odd.

## Purpose and patterns

`polybee` simulates the foraging movement of a population of bees around
one or more hives, within an environment that may contain a rectangular
polytunnel (with netted or open entrances), linear barriers, and patches
of flowering plants. For a given, fixed physical configuration of these
elements, it generates the emergent spatial and temporal patterns of bee
activity -- where bees spend time, how successfully flowers are
pollinated, and how easily bees cross netted tunnel boundaries -- that
result from simple individual foraging, homing, and obstacle-avoidance
rules. The model itself considers only a single, fixed configuration; it
is designed to serve as the evaluation function for a separate
*environment-shaping* optimisation process (@sec:environment-shaping-in-abms
and the Experiments section) that searches over candidate configurations
to steer these patterns towards a target.

Three patterns are used to judge the model's adequacy. Bee visitation
should concentrate plausibly around plant patches and accessible tunnel
entrances or corridors, and respond sensibly to hive, entrance, and
barrier placement. The per-attempt exit probabilities and maximum-attempt
counts governing tunnel-netting permeability were calibrated to reproduce
the exit-success rate and mean rebound count reported for real netted
enclosures in @sonter2024. And the fraction of flowers receiving a
"successful" number of visits should decrease appropriately when flowers
are cut off from a hive by barriers or by an unfavourable entrance
configuration.

## Entities, state variables and scales

The model's entities are an environment containing a polytunnel,
entrances, barriers, and patches of flowering plants; one or more hives;
and a population of bees that forage from the plants and periodically
return to their hive.

**Environment model.** The environment is a rectangular, continuous 2D space of
arbitrary units, not tied to an explicit real-world measure such as metres,
although the parameters `bee-step-length` and `bee-visual-range` calibrate it
indirectly. It may contain a single rectangular polytunnel with one or more
entrances, each either open or fitted with anti-bird or anti-hail netting, and
any number of linear barriers. Positions and movement are continuous; grid cells
are used only for efficient spatial lookup and for output aggregation, not for
the movement dynamics themselves.

**Bee agent model.** Each bee maintains its current position and heading;
an energy level that rises when it feeds and falls with each foraging
step; a behavioural state (foraging, on a flower, returning to the hive
inside or outside the tunnel, or in the hive); and a short rolling memory
of the five most recently visited plants, which it will not revisit.
Each bee is linked to one home hive.

**Hive model.** Each hive has a fixed position and an entry/exit
direction (one of the four compass points, or random), which sets the
heading a bee adopts whenever it leaves or arrives at the hive. Hives
have no dynamics of their own; they are fixed spawn/return points, and the
bee population is divided evenly across however many are configured.

**Plant model.** Each plant (flower) has a fixed position and a nectar
store, initialised to a fixed value and depleted, never replenished, as
bees extract it. Each plant also records the number of times it has been
visited, used to assess pollination success (see Purpose and patterns,
above).

## Process overview and scheduling

At the start of a run the environment is built once: the polytunnel and
its entrances, any barriers, and patches of plants (laid out as jittered
regular grids) are created and spatially indexed; hives and, divided
evenly across them, the configured number of bees are then created. Each
iteration, every bee is updated once, in creation order (harmless, since
bees never interact with each other directly); the heatmap records each
bee's current cell; and, periodically, the flowmap records each bee's
movement direction. A run ends after a fixed number of iterations.

**Submodel: bee movement and foraging.** When no unvisited flower is
within a bee's visual range, or with fixed probability it chooses not to
head for one it has found, it takes a short random-walk step, turning by
a bounded random amount from its current heading:
$$
\theta_{t+1} = \theta_t + U(-\delta_{\max}, \delta_{\max})
$$
where $\delta_{\max}$ is a fixed maximum turning rate per step (a
correlated random walk). The decision each step is:

```
If an unvisited flower is within visual range:
    with fixed probability, head directly for the nearest such flower
    otherwise, take a bounded random turn
Else:
    take a bounded random turn
```

Heading directly for a sensed flower is the one case where a bee's turn
is unconstrained by $\delta_{\max}$. On reaching a flower, the bee
extracts nectar (capped by however much remains), its visit count
increments, and it remains on the flower for a fixed number of iterations
before resuming foraging.

**Submodel: tunnel and barrier interactions.** A candidate step obstructed
by a barrier is either flown over (fixed probability), shortened to stop
just short of the barrier, or abandoned in favour of a new random
direction. A candidate step that would cross the tunnel boundary through
an entrance succeeds or fails as an independent trial at that entrance's
per-attempt exit probability (1.0 for an open entrance, $\approx 0.12$ for
anti-bird netting, $\approx 0.04$ for anti-hail netting, calibrated from
@sonter2024); on repeated failure the bee rebounds off the netting and
re-attempts, up to a fixed maximum number of attempts, before giving up
and continuing to forage on its current side of the tunnel. A move that
meets solid tunnel wall, or the outer boundary of the environment, is
clamped to slide along it rather than reflecting off it.

**Submodel: energy dynamics and return to hive.** A bee's energy falls by
a fixed amount each foraging step and rises, nectar-limited, on each
flower visit. Once energy leaves a fixed band, the bee switches to
returning directly to its hive, taking the shortest route that avoids
passing back through solid tunnel wall (routing via the tunnel's corners
if necessary). Having reached the hive, the bee rests there for a fixed
number of iterations, after which its energy resets and it resumes
foraging.

## Design concepts

**Basic principles.** Bees follow a correlated random walk biased towards
nearby, unvisited flowers, rather than a true Lévy flight. An energy
budget accumulated from nectar and depleted per step governs the length
of a foraging bout and the decision to return to the hive. Tunnel-netting
permeability is represented stochastically, with exit probabilities taken
directly from published field-trial data on real netting [@sonter2024]
rather than from a physical model of the netting itself.

**Emergence.** The population-level patterns of interest -- the
visitation heatmap, per-plant visit counts and the resulting
pollination-success fraction, and the overall movement flow field -- are
not encoded directly anywhere in the model. They emerge from many bees
independently applying the same simple foraging, obstacle-avoidance, and
homing rules within one particular, fixed spatial arrangement of hive(s),
tunnel, entrances, barriers, and plant patches.

**Adaptation, objectives, learning and prediction.** None of these are
modelled. Bees follow fixed decision rules irrespective of experience;
their only memory, the rolling list of recently visited plants, exists
purely to avoid immediate revisits, not to adapt future behaviour. The
energy state variable creates an implicit approach/avoidance dynamic
around a fixed threshold band, but this is a hard-coded rule rather than
an objective the bee evaluates or optimises. Bees do not anticipate
future states; every movement decision depends only on current position,
state, and what is currently sensed.

**Sensing.** A bee senses unvisited flowers within a fixed visual range of
its current position, excluding those in its recent-visit memory and any
whose line of sight is blocked by a wall or barrier, and any barrier that
would obstruct its next candidate step. It does not sense other bees, the
hive from a distance, or any aggregate/global state.

**Interaction.** Bees do not interact with one another directly -- there
is no collision avoidance, communication, or recruitment. The only
channel by which one bee's actions affect another is indirect, through
shared plant state: one bee extracting nectar from, or visiting, a flower
changes what the next bee to arrive there will experience. Bees do
interact with the static environment -- tunnel walls and netted
entrances, barriers, and the hive.

**Stochasticity.** All randomness derives from a single seeded
random-number stream. Stochastic elements include: a bee's initial/
hive-exit heading (when a hive's direction is configured as random); the
turning noise added on each random-walk step; the choice between heading
for a sensed flower and taking a random turn instead; the outcome of each
tunnel-entrance crossing attempt; whether a barrier is flown over or
avoided; small positional jitter while homing; and the initial jittered
placement of plants within their patches.

**Collectives.** None. Bees are not organised into any collective
structure such as a swarm or division of labour; a hive is simply a
shared spawn/return point, and the bee population is divided evenly
across however many hives are configured.

**Observation.** The main outputs recorded are a heatmap of bee visitation
density and a flowmap of predominant local movement direction, both accumulated
over a run; per-plant visit counts, from which the successful visit fraction is
derived; and summary statistics on tunnel-entrance crossing attempts (success
rate and mean number of rebounds). None of these outputs are read by the bees
themselves. Optionally, a reference bee-position heatmap can be supplied by the
user and used as a target for the optimisation process instead of the usual
successful visit fraction; in this case, the system also outputs the _Earth
Mover's Distance_ (EMD) measure between the bee-position heatmap produced by the
simulation and the reference heatmap supplied (`polybee` uses the `OpenCV`
implementation of the EMD calculation for this purpose).

## Initialization

A run is initialised from a set of named parameters, read from a
configuration file and optionally overridden from the command line. The
tunnel, its entrances, any barriers, and patches of plants (laid out as
regular grids with independent positional jitter) are constructed first;
one or more hives are then placed, and a fixed number of bees created and
divided evenly across them, each starting at its hive with a full energy
budget. A single random-number stream is seeded once for the whole run,
either from a supplied seed or a freshly generated one, recorded for
reproducibility.


# Experiments

## Design

### Base environment

All four experimental conditions below (the three evolved conditions and
the baseline) share the same fixed physical environment, differing only
in whether barriers and/or bridges are present. An example of the basic environmental
configuration is shown in @fig:basic-env-screenshot.

![Screenshot showing the basic environmental configuration used in the
experiments. A central tunnel (the dark brown area) houses four rows of crops. A
single un-netted entrance is provided on the southern end of the tunnel (thick
white line), and a single hive (red square) is placed outside the tunnel in
front of the entrance. The hive has a single entrance/exit facing north (i.e.
towards the tunnel). The tunnel is surrounded by a perimeter area (light
brown). Fifty bees (small triangles) forage over the environment for 2000
simulation timesteps. In this visualisation, the colour of each plant indicates
how often it has been visited by bees, with light green indicating the most
visited
plants.](figures/polybee-env-config-example-screenshot.png){#fig:basic-env-screenshot
width=70%}

The environment is $650 \times 800$ units,
containing a single rectangular polytunnel $450 \times 600$ units, with
its top-left corner at $(100, 100)$ (so the tunnel spans $x \in [100,
550]$, $y \in [100, 700]$). The tunnel has a single, unnetted entrance on
its south wall: a 100-unit-wide opening centred on the wall's midpoint
($x \in [275, 375]$ at $y = 700$). A single hive sits 50 units south of
(i.e. just outside) the tunnel, at $(325, 750)$, oriented to face north,
directly towards the entrance.

Four parallel rows of flowering plants run the length of the tunnel:
each row is a $50 \times 500$ unit patch, evenly spaced 100 units apart
($x = 150, 250, 350, 450$), spanning $y \in [150, 650]$, well inside the
tunnel. Within each patch, plants are laid out on a 10-unit grid with a
small amount of positional jitter (0.1 s.d.). Fifty bees forage from the
hive over 2000 iterations per simulation run.

Barriers, where present, only partially obstruct movement: a bee blocked
by one has a fixed 10% chance (`barrier-pass-prob`) of flying over it
anyway rather than being diverted.

@tbl:params lists the baseline parameter values used across all of the
experiments reported here unless otherwise stated.

| Parameter | Meaning | Value |
|---|---|---|
| `env-width`, `env-height` | Environment extent | 650, 800 |
| `tunnel-width`, `tunnel-height` | Tunnel extent | 450, 600 |
| `tunnel-x`, `tunnel-y` | Top-left position of tunnel | 100, 100 |
| `tunnel-​entrance` | Entrance span, side, net type | `175,275:2:0` |
| `barrier-​pass-​prob` | Probability of flying over a barrier | 0.1 |
| `flower-​initial-​nectar` | Initial nectar per flower | 100 |
| `plant-​default-​spacing` | Default plant spacing | 10.0 |
| `plant-​default-​jitter` | Default plant position jitter | 0.1 |
| `hive` | Hive position and direction | `325,750:0` |
| `num-bees` | Number of bees | 50 |
| `bee-​max-​dir-​delta` ($\delta_{\max}$) | Max heading change per step (radians) | 0.5 |
| `bee-​step-​length` ($\ell$) | Distance moved per step | 15 |
| `bee-​path-​record-​len` | Max path positions retained per bee | 0 |
| `bee-​visual-​range` | Max distance at which a bee can detect a flower | 11 |
| `bee-​visit-​memory-​length` | Recently visited plants remembered | 5 |
| `bee-​prob-​visit-​nearest-​flower` ($p_{\text{nearest}}$) | Probability of heading for a sensed flower | 0.8 |
| `bee-​in-​hive-​duration` | Iterations spent in hive between bouts | 200 |
| `bee-​initial-​energy` | Energy on leaving the hive | 500 |
| `bee-​energy-​depletion-​per-​step` | Energy cost per step | 1 |
| `bee-​energy-​boost-​per-​flower` | Energy gained per flower visit | 10 |
| `bee-​on-​flower-​duration` | Iterations spent on a flower | 3 |
| `bee-​energy-​min-​threshold` | Energy floor triggering return to hive | 0 |
| `bee-​energy-​max-​threshold` | Energy ceiling triggering return to hive | 750 |
| `num-​iterations` | Number of simulation iterations | 2000 |
| `heatmap-​cell-​size` | Heatmap cell size | 50 |
| `min-​visit-​count-​success` | Lower bound of a "successful" visit count | 3 |
| `max-​visit-​count-​success` | Upper bound of a "successful" visit count | 1000 |

: Baseline parameter values used across the experiments reported here,
taken from `baseline-runs-2000its.cfg` (see @sec:exptconds). Parameters
controlling only the evolutionary search layer (e.g. `evolve-spec`,
`num-generations`) vary by condition and are given in
@sec:evosearch instead; see @tbl:appendix-params for parameter
descriptions and software defaults. {#tbl:params}

### Experimental conditions {#sec:exptconds}

Four conditions were run:

- **Baseline.** No barriers or bridges, and no evolutionary search --
  just the base environment above, run 5000 times independently (matching
  the 50 replicates $\times$ 100 trials-per-configuration scale of each
  evolved condition below, so the samples are of comparable size).
- **Evolve 20 barriers.** The positions of 20 barriers were evolved;
  entrance and hive positions were held fixed. Each barrier is a
  fixed-length (50-unit) line segment; the search evolves each barrier's
  midpoint position (continuous $x, y$) and orientation (discretised to
  36 possible angles, in $10^\circ$ steps). Barrier placement was
  freely evolved; no attempt was made to prevent placement of barriers that
  crossed the tunnel walls or other solid objects such as the hive.
- **Evolve 10 bridges.** The positions of 10 "bridges" were evolved. A
  bridge is implemented as a small ($30 \times 30$ unit) patch of
  flowering plants, which could represent a large pot. The idea was that
  these bridges, when placed between crop rows, might encourage the bees
  to switch rows or otherwise alter their foraging behaviour.
  Flowers within a bridge patch are excluded from the
  pollination-success fitness metric itself (below) -- they exist purely
  as a movement aid, not as a crop to be pollinated.
- **Evolve 20 barriers and 10 bridges.** Both of the above evolved
  simultaneously, within the same search.

### Evolutionary search {#sec:evosearch}

For each of the three evolved conditions, barrier and/or bridge positions (and
barrier orientations) were searched for using a genetic algorithm, run for 400
generations with a population of 400 candidate configurations per generation.
Each candidate configuration was evaluated over 100 independent, stochastic
simulation trials (2000 iterations each); its fitness was derived from the
median, across those 100 trials, of the fraction of flowers receiving a
"successful" number of visits. Any flower that is visited at least 3 times
during a simulation is counted as "successfully viited" for this purpose
(`min-visit-count-success=3`).

The genetic algorithm functionality of `polybee` is implemented using the `sga`
component of the `pagmo` library [@pagmo2020]. `pagmo` is an extensive library
of optimisation methods that uses a consistent goal of minimisation across its
various tools. For this reason, in all our experiments we actually employed the
goal of _minimising the negated visit fraction_ (so the best possible value is
-1.0 rather than 1.0). Therefore, where negative visit fractions are presented
in the @sec:results, it should be remembered that more
negative values (closer to -1.0) are better than less negative ones (closer to
0.0).

Each evolutionary condition was replicated 50 times, each replicate an
independent run of the full 400-generation search with its own randomly
generated seed, run as a 50-task array job on the Monash M3 HPC cluster
(see the accompanying `.slurm` files alongside each condition's raw
output).

Some further technical comments about the configuration of the evolutionary
search in these experiments can be found in
@sec:appendix-misc-tech-notes.

## Setup {#sec:setup}

### Per-condition analysis

Each condition's raw output was analysed independently with
`tools/run_analysis.sh`, run from a directory containing that condition's
`raw-output`. This tool performs the following actions:

1. extracts per-generation fitness values and the champion (best) fitness
   per run into CSVs, and produces per-run and aggregate fitness graphs;
2. extracts the best-performing individual's configuration from each
   replicate run into its own `.cfg` file;
3. builds barrier/bridge position heatmaps and barrier flowmaps from
   those best-individual configs, at three cell sizes (10, 25, 50);
4. re-runs each replicate's best-individual configuration through
   `polybee` to record bee-movement flowmaps and position heatmaps at
   each of those three cell sizes, then merges these across the 50
   replicates (mean per cell, for heatmaps).

For the baseline condition, `run_analysis.sh` is passed the `--baseline` flag in
place of `--num-reps`: since baseline runs are single, non-evolutionary
simulations (`evolve=false`) rather than evolutionary searches, there are no
generations or best-individual configs to extract, and no barriers or bridges to
map (steps 1 and 3 above are skipped). Instead, `run_analysis.sh` treats each of
the 5000 raw output files as one run, taking its "fitness" from the run's own
successful-visit fraction (negated, for a like-for-like comparison with the
evolved conditions) and its "best-individual config" as simply its own logged
configuration; per-run flowmaps and heatmaps are then merged in the same way as
for the evolved conditions. All of the heatmaps used in this analysis are normalised, so the process of creating
merged heatmaps is straightforward. For details of how the flowmaps are generated and merged, see
Appendix D (@sec:appendix-flowmap-merging).

The exact invocations used were:
```
tools/run_analysis.sh \
    --basename evolve-10B-400gen-400pop-100epi-2000its \
    --num-reps 50 \
    --title "Evolve positions of 10 bridges"

tools/run_analysis.sh \
    --basename evolve-20X-400gen-400pop-100epi-2000its \
    --num-reps 50 \
    --title "Evolve positions of 20 barriers"

tools/run_analysis.sh \
    --basename evolve-20X-10B-400gen-400pop-100epi-2000its \
    --num-reps 50 \
    --title "Evolve positions of 20 barriers and 10 bridges"

tools/run_analysis.sh \
    --basename baseline-runs-2000its \
    --baseline \
    --title "Baseline [no barriers or bridges] (5000 runs)"
```
(default plot-scale, flowmap-threshold, and heatmap-cell-size options
were used throughout; see `run_analysis.sh --help` for the full set of
overrides available).

### Cross-condition analysis {#sec:setup-cross-condition-analysis}

Pairs of conditions were then compared with `tools/run_cross_analysis.sh`,
which compares two `run_analysis.sh` output directories' merged
bee-movement flowmaps and merged bee-position heatmaps, for each cell
size present in both:

- **Flowmap comparison.** For each cell size, an axial angular-delta heatmap is
  computed cell-by-cell between the two conditions'
  predominant-movement-direction flowmaps using the standard angle-doubling
  transformation for axial data: $$ \Delta =
  \tfrac{1}{2}\arccos\bigl(\cos(2(\alpha_1 - \alpha_2))\bigr) $$ where
  $\alpha_1, \alpha_2$ are the two conditions' flowmap axis angles for that cell
  (treated as headless, i.e. a direction and its opposite are equivalent);
  $\Delta$ ranges from 0 (identical axis) to $\pi/2$ (perpendicular axes). This
  is computed twice per cell size: once with no thresholding, and again with
  default strength and count thresholds of 0.5 and 0.1, respectively.
  Angular-delta heatmap visualisation uses a configuration file taken from the
  first condition's `best-configs-indiv/`, on the assumption that both
  conditions share the same basic environment -- true of all the comparisons run
  here, since all four conditions share the base environment described above.
- **Heatmap comparison.** A plain cell-by-cell difference (first
  condition's normalised visitation heatmap minus the second's) is
  computed once per cell size, with no thresholding, and visualised with
  a diverging colour scale.

The exact invocations used were:
```
tools/run_cross_analysis.sh \
    --delta-color-scale-max 0.004 \
    --title "Evolve positions for 20 barriers and 10 bridges vs Baseline" \
    evolve-20X-10B-400gen-400pop-100epi-2000its \
    baseline-runs-2000its

tools/run_cross_analysis.sh \
    --delta-color-scale-max 0.004 \
    --title "Evolve positions for 20 barriers vs Baseline" \
    evolve-20X-400gen-400pop-100epi-2000its \
    baseline-runs-2000its

tools/run_cross_analysis.sh \
    --delta-color-scale-max 0.004 \
    --title "Evolve positions for 10 bridges vs Baseline" \
    evolve-10B-400gen-400pop-100epi-2000its \
    baseline-runs-2000its

tools/run_cross_analysis.sh \
    --delta-color-scale-max 0.004 \
    --title "Evolve positions for 20 barriers and 10 bridges vs 20 barriers only" \
    evolve-20X-10B-400gen-400pop-100epi-2000its \
    evolve-20X-400gen-400pop-100epi-2000its

tools/run_cross_analysis.sh \
    --delta-color-scale-max 0.004 \
    --title "Evolve positions for 20 barriers and 10 bridges vs 10 bridges only" \
    evolve-20X-10B-400gen-400pop-100epi-2000its \
    evolve-10B-400gen-400pop-100epi-2000its
```
The line `--delta-color-scale-max 0.004` fixes the colour scale for the bee
position delta comparison to $\pm 0.4\%$ (0.004 is specified as
a fraction). For reference, for heatmaps with cell size 50x50, as used for the
bee position delta heatmaps in @fig:bee-heatmap-baseline-vs-deltas, there are
16x13 cells in the heatmap (as the environment is 800x650 units), meaning that
each cell in a bee position heatmap with completely homogeneous occupancy would
have a value of $1/(16*13)=0.00481$ ($0.481\%$). Fixing the colour scale range
to $\pm 0.004$ is therefore of a similar magnitude to this value, and we
determined empirically that it was a good value for visualising and
differentiating interesting features of the delta heatmaps.

# Results and analysis {#sec:results}

## Examples of evolved barrier and bridge configurations

@fig:best-layouts shows, for each evolved condition, the physical
layouts (barrier and/or bridge placement) of the three top-scoring
configurations across the 50 replicate runs. Tunnel wall, crop rows, and hive
are shown for reference in each panel.

The figure shows that for the "Evolve 10 bridges" condition (top row), the best
configurations were those in which most of the bridges were situated within
the tunnel at the top end, towards the end of the middle two rows of crop. In addition,
some of the bridges were placed at the other end of the environment, outside the
tunnel and just behind the hive.

For the "Evolve 20 barriers" condition (middle row), the best configurations exhibit
a central "funnel" configuation at the near end of the rows (close to the tunnel entrance),
together with barriers blocking the near end of the middle two rows, and a line of barriers
on the outer edges of the leftmost and rightmost rows.

For the "Evolve 20 barriers and 10 bridges" condition (bottom row), the barrier positions
resembled those seen in the "Evolve 20 barriers" condition but with less clearly defined features.
In one case (the middle one) the bridge positions resembled those seen in the "Evolve 10 bridges" condition,
but in the other two cases most of the bridges were actually placed _outside_ the tunnel at the
top end.


::: {#fig:best-layouts}
![Bridges only, rank 1](figures/best-layout-1-evolve-10B-400gen-400pop-100epi-2000its-58270372_12.png){#fig:best-layout-10B-1 width=30%}
![Bridges only, rank 2](figures/best-layout-2-evolve-10B-400gen-400pop-100epi-2000its-58270372_22.png){#fig:best-layout-10B-2 width=30%}
![Bridges only, rank 3](figures/best-layout-3-evolve-10B-400gen-400pop-100epi-2000its-58270372_30.png){#fig:best-layout-10B-3 width=30%}

```{=latex}
\mbox{}\\
```

![Barriers only, rank 1](figures/best-layout-1-evolve-20X-400gen-400pop-100epi-2000its-58163058_31.png){#fig:best-layout-20X-1 width=30%}
![Barriers only, rank 2](figures/best-layout-2-evolve-20X-400gen-400pop-100epi-2000its-58163058_18.png){#fig:best-layout-20X-2 width=30%}
![Barriers only, rank 3](figures/best-layout-3-evolve-20X-400gen-400pop-100epi-2000its-58163058_20.png){#fig:best-layout-20X-3 width=30%}

```{=latex}
\mbox{}\\
```

![Barriers and bridges, rank 1](figures/best-layout-1-evolve-20X-10B-400gen-400pop-100epi-2000its-58020874_34.png){#fig:best-layout-20X10B-1 width=30%}
![Barriers and bridges, rank 2](figures/best-layout-2-evolve-20X-10B-400gen-400pop-100epi-2000its-58020874_24.png){#fig:best-layout-20X10B-2 width=30%}
![Barriers and bridges, rank 3](figures/best-layout-3-evolve-20X-10B-400gen-400pop-100epi-2000its-58020874_46.png){#fig:best-layout-20X10B-3 width=30%}

The three best-performing evolved layouts (ranked by best-observed
fitness across replicate runs) for each experimental condition:
bridges-only (top row), barriers-only (middle row), and
barriers-and-bridges (bottom row). Barriers are drawn as red line segments
and bridges as purple squares.
:::

**TODO** Make the hives more visible in these plots

## Overall fitness statistics {#sec:overallstats}

@tbl:fitness-summary shows the flower visitation summary statistics for each
experimental condition. The figures in the second column show the median of the
best observed visitation fraction for all replicate runs under the given
condition, where the visitation fraction is the fraction of all plants in the
environment that were visited by bees at least three times during the
simulation. Note again thet in all reported results, the fitness number is negated,
so lower numbers (i.e. closer to -1.0) are better (see @sec:evosearch).

We report the IQR (interquartile range) as our measure of variability between
replicate runs, because for the evolve conditions we are looking at the results
across 50 replicate runs; as this is not a very large number, the 10th--90th
band can be unstable, so the IQR is more robust.  Note that for the baseline
condition we are looking at 5000 independent runs (see @sec:exptconds); there is
no inherent problem in comparing medians and IQRs across these very different
sample sizes in the different conditions (5000 vs 50), although the figures for
the baseline condition will be a better estimate of the underlying population
statistics.

| Condition | Median Fitness | IQR (Q3 - Q1) |
|---|---:|---:|
| Baseline (no barriers or bridges) | -0.5770 | 0.0400 |
| Bridges only (10 bridges) | -0.5945 | 0.0025 |
| Barriers only (20 barriers) | -0.7490 | 0.0069 |
| Barriers and bridges (20 barriers + 10 bridges) | -0.7353 | 0.0118 |

: Median and interquartile range (IQR) of the best achieved negated successful
visit fraction across the 50 replicate runs of each condition (more negative is
better). {#tbl:fitness-summary}

To test whether each evolved condition's median fitness differs significantly
from the baseline's, given the very different sample sizes involved (5000
baseline runs vs. 50 replicates per evolved condition) and the non-normal,
bounded nature of the fitness metric, we used a nonparametric bootstrap rather
than a parametric test such as a t-test: each pair of samples (one evolved
condition vs. baseline) was resampled independently, each at its own size,
10,000 times, to obtain a 95% confidence interval for the difference in
medians and a two-sided bootstrap hypothesis-test p-value, with Holm-Bonferroni
correction applied across the three evolved-vs-baseline comparisons. Full
methodology is given in @sec:appendix-bootstrap. As @tbl:fitness-significance
shows, all three evolved conditions differ significantly from the baseline
(Holm-adjusted $p = 0.0003$ in every case), with confidence intervals for the
median difference that exclude zero by a wide margin -- consistent with the
much larger effect sizes for the two barrier-involving conditions than for
bridges alone.

| Condition | Median diff. vs. baseline | 95% CI | Holm-adjusted $p$ |
|---|---:|---:|---:|
| Bridges only (10 bridges) | -0.0175 | [-0.0190, -0.0163] | 0.0003 |
| Barriers only (20 barriers) | -0.1720 | [-0.1738, -0.1693] | 0.0003 |
| Barriers and bridges | -0.1583 | [-0.1615, -0.1540] | 0.0003 |

: Bootstrap 95% confidence intervals and Holm-Bonferroni-adjusted two-sided
p-values for the difference between each evolved condition's median fitness
and the baseline's (negative = evolved condition better than baseline, since
lower/more-negative fitness is better; see @sec:appendix-bootstrap).
{#tbl:fitness-significance}


## Evolution of fitness over generations

@fig:fitnessovergens shows how the fitness (negated successful visit fraction) evolved
over the generations in the three Evolve conditions. It is apparent from these
graphs that the evolution of bridge positions (@fig:fitnessovergens-10B) was not very
amenable to evolutionary optimisation, with negligable improvement in fitness after the
first 50 generations. By contrast, the evolution of barriers (@fig:fitnessovergens-20X) showed
continued improvement over the whole 400-generation run. Combining the evolution of barriers
with bridges (@fig:fitnessovergens-20X10B) actually led to _worse_ results than just
evolving bridges by themselves. This is consistent with the summary statistics reported in
@sec:overallstats.

::: {#fig:fitnessovergens}
![No evolutionary run for baseline expt](figures/blank-placeholder.png){#fig:fitnessovergens-baseline width=45%}
![Evolve Bridges only](figures/graph-fitness-evolve-10B-400gen-400pop-100epi-2000its-summary.png){#fig:fitnessovergens-10B width=45%}

```{=latex}
\mbox{}\\
```

![Evolve Barriers only](figures/graph-fitness-evolve-20X-400gen-400pop-100epi-2000its-summary.png){#fig:fitnessovergens-20X width=45%}
![Evolve barriers and bridges](figures/graph-fitness-evolve-20X-10B-400gen-400pop-100epi-2000its-summary.png){#fig:fitnessovergens-20X10B width=45%}

Evolution of (negated) successful visit fraction over time in the
evolutionary runs for each condition (there being no evolutionary run for
the baseline condition, panel (a) is blank). In each panel, the upper
(gold) line shows the median across the 50 replicate runs of the median
score across 400 individuals in each generation; the lower (green) line
shows the median across the 50 replicate runs of the best (minimum) score
across 400 individuals in each generation. The shaded areas around the
lines show the IQR (interquartile range) across the 50 runs.
:::


## Flowfield analysis of bee movements under each condition {#sec:flowfield-analysis}

To understand how the bees are moving around their environment, and how the introduction
of barriers and/or bridges changes their movement patterns, we produced flowmaps
showing the axial flow of bees in each part of the environment across the course of
a simulation. The axial flow treats 0^o^ and 180^o^ as equivalent, and is therefore
good for showing whether bees are being channelled along the same routes in the environment.

@fig:bee-flowmap-size-10-all-conditions shows the flowmaps for each of the experimental
conditions. Each of these is itself a merge of many independent per-replicate
flowmaps; see Appendix D (@sec:appendix-flowmap-merging) for exactly how these
per-replicate flowmaps are generated and merged.

In each flowmap, the colour of a line in a given position represent how strongly aligned the
recorded bee movements were at that position --- a red line indicates that nearly all recorded
bees were moving in the same axial direction, whereas a dark blue line indicates that bees were
moving in many different directions. The thickness of each line indicates how many bees were
recorded at the position during the course of a simulation --- the thicker then line, the
more bees recorded.

The flowmaps show that the bees generally moved parallel to the edges of the tunnel and of the
outer perimeter of the environment when close to these edges, but in corners (of the tunnel
and of the environment) they tended to move in a less homogeneous direction (blue areas in
corners of flowmaps). Bees emerging from the hive did so in a very homogeneous direction (red
lines), which is expected as the hive had a single exit which faced the tunnel entrance.
Over the crop patches the bees moved in heterogeneous directions (blue) as dictated by their
foraging behaviour. In between crop rows, the bee's movement tended to get less homogeneous
the further they travelled down the tunnel away from the entrance (flowlines transition from
greens to blues), and fewer bees were recorded at the far end of the tunnel (thiner lines
than nearer the entrance).

In the conditions involving evolved bridges (@fig:bee-flowmap-sz10-10B and
@fig:bee-flowmap-sz10-20X10B), the bees' movements immediately behind the hive
(bottom centre of the figures) was less uniform (more blue than green),
indicating that the bees were not simply flying past this area but were stopping
on the bridge patches that were positioned in that area (@fig:best-layout-10B-1
-- @fig:best-layout-10B-3).

In the conditions involving evolved barriers (@fig:bee-flowmap-sz10-20X and
@fig:bee-flowmap-sz10-20X10B), the bees' movements just inside the entrance to
the tunnel were a little less uniform than in the baseline (green rather than
yellow), indicating that they were not simply flight straight ahead but had to
change direction because of the bridges.

::: {#fig:bee-flowmap-size-10-all-conditions}
![Baseline (no barriers or bridges)](figures/bee-flowmap-size-10-intra-condition-merged-baseline-runs-2000its.png){#fig:bee-flowmap-sz10-baseline width=45%}
![Evolve 10 bridges only](figures/bee-flowmap-size-10-intra-condition-merged-evolve-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz10-10B width=45%}

```{=latex}
\mbox{}\\
```

![Evolve 20 barriers only](figures/bee-flowmap-size-10-intra-condition-merged-evolve-20X-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz10-20X width=45%}
![Evolve 20 barriers and 10 bridges](figures/bee-flowmap-size-10-intra-condition-merged-evolve-20X-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz10-20X10B width=45%}

Flowmaps showing predominant directions of bee movement, aggregated over
5000 independent baseline runs (a), and over 50 runs of each replicate's
champion configuration in the bridges-only (b), barriers-only (c), and
barriers-and-bridges (d) conditions. Colour indicates the strength of
alignment of bee movement at each position (colour scale at right); line
thickness indicates how many bees were recorded at that position over the
course of a simulation.
:::

## Analysis of change in bee movements caused by barriers and bridges {#sec:analysis-of-change-in-bee-movements-caused-by-barriers-and-bridges}

To get a better idea of how the different conditions changed the movement
patterns of the bees, we looked at angular delta heatmaps showing the difference
between the flowmaps for each of the Evolve conditions and the baseline flowmap.
For each cell, this "angular delta" is the axial difference between that
condition's predominant movement axis and the baseline's in that cell: 0 where
bees in the two conditions moved (or exactly counter-moved) along the same axis
through that cell, up to 90 degrees where their predominant directions were
perpendicular.

@fig:bee-flowmap-sz10-baseline2 is just a
reproduction of the flowmap for the baseline condition shown in
@fig:bee-flowmap-sz10-baseline. The other three panels in the figure show the
angular delta heatmaps, indicating the _difference_ observed in the bees'
direction of movement, compared to the baseline, for each of the three Evolve
conditions. Thresholds have been applied to each cell in these
heatmaps to exclude cases where (1) the strength of alignment in that cell in
both of the flowmaps being compared is too low (`strength-th`=0.250); (2) not
enough data was present for that cell in either of the flowmaps being compared
(`count-th`=0.025); and (3) the computed angular delta between the two
flowmaps for that cell was too low (`delta-th`=0.175 radians, about 10^o^).
Cells that fall below any of these thresholds are shown in white in the angular
delta heatmaps. For further information on the calculation of the angular delta
heatmaps and  the exclusion criteria used, see Appendix E
(@sec:appendix-angdelta).


**TODO** Write a brief descripition of the results


::: {#fig:angdelta-heatmap-size-10-all-conditions}
![Baseline bee movements](figures/bee-flowmap-size-10-intra-condition-merged-baseline-runs-2000its.png){#fig:bee-flowmap-sz10-baseline2 width=45%}
![Angular deltas for Evolve 10 bridges only](figures/angdelta-heatmap-sz10-st-0p250-ct-0p025-dt-0p175-cross-analysis-evolve-10B-vs-baseline.png){#fig:angdelta-heatmap-sz10-10B width=45%}

```{=latex}
\mbox{}\\
```

![Angular deltas for Evolve 20 barriers only](figures/angdelta-heatmap-sz10-st-0p250-ct-0p025-dt-0p175-cross-analysis-evolve-20X-vs-baseline.png){#fig:angdelta-heatmap-sz10-20X width=45%}
![Angular deltas for Evolve 20 barriers and 10 bridges](figures/angdelta-heatmap-sz10-st-0p250-ct-0p025-dt-0p175-cross-analysis-evolve-20X-10B-vs-baseline.png){#fig:angdelta-heatmap-sz10-20X-10B width=45%}

Analysis of change in bee movements caused by barriers and bridges.
:::


## Effect of barriers and bridges on bee coverage of the environment {#sec:effect-of-barriers-and-bridges-on-bee-coverage-of-the-environment}

**TODO** redo plots in @fig:bee-heatmap-baseline-vs-deltas so that red
indicates a positive delta and blue negative, so it is consistant with
the scales on the other maps. And change all of these plots to show a better range of the data
(e.g. a non-linear scale or clipped scale?)

In @sec:flowfield-analysis and
@sec:analysis-of-change-in-bee-movements-caused-by-barriers-and-bridges we
looked at how bee movement was affected by the presence of barriers and bridges.
In this section we look at the presence of bees, rather than their direction of
movement, in specific locations throughout the simulations.

@fig:bee-heatmap-baseline-vs-deltas(a) shows a normalised bee-position heatmaps
for the baseline condition: for each of the 5000 runs under this condition, the
total number of bee-position observations recorded in each cell, summed over all
2000 simulation steps, is normalised so all cells sum to 1.0; the merged heatmap
for a condition is then the mean, cell by cell, of that normalised heatmap
across all runs of the condition (cell size 50).
@fig:bee-heatmap-baseline-vs-deltas(b)--(d) instead show, for each evolved
condition, the per-cell difference between that condition's own heatmap and the
baseline's (evolved minus baseline, so positive/red cells were visited
relatively more under the evolved condition, negative/blue cells relatively
less), on a diverging colour scale fixed to $\pm 0.4\%$, as
described in @sec:setup-cross-condition-analysis.

::: {#fig:bee-heatmap-baseline-vs-deltas}
![Baseline occupancy (merged across 5000 runs)](figures/bee-heatmap-size-50-intra-condition-merged-baseline-runs-2000its-cropped.png){#fig:bee-heatmap-baseline-abs width=45%}
![Delta occupancy for Evolve 10 bridges only](figures/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-10B-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs.png){#fig:bee-heatmap-delta-10B width=45%}

```{=latex}
\mbox{}\\
```

![Delta occupancy for Evolve 20 barriers only](figures/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-20X-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs.png){#fig:bee-heatmap-delta-20X width=45%}
![Delta occupancy for Evolve 20 barriers and 10 bridges](figures/bee-heatmap-delta-bee-heatmap-size-50-intra-condition-merged-evolve-20X-10B-vs-bee-heatmap-size-50-intra-condition-merged-baseline-runs.png){#fig:bee-heatmap-delta-20X10B width=45%}

Normalised bee-position coverage of the environment: the baseline heatmap
(a), and its per-cell difference against each evolved condition --
bridges-only (b), barriers-only (c), and barriers-and-bridges (d).
:::


## Discussion

Interpretation, limitations, surprises.

**TO DO**

# Conclusion

Summary of findings and future work.

**TO DO**

# Appendix A: Full ODD Description of the PolyBee Simulation Model {#sec:appendix-full-odd}

This appendix gives the complete, submodel-level ODD description of the
`polybee` simulation, in full implementation detail (every state
variable, submodel equation, and the full parameter reference). A
condensed summary appears in the main text (see @sec:system-design).

The `polybee` simulation is described below following the ODD (Overview,
Design concepts, Details) protocol for describing agent-based models
[@grimm2020odd]. This description covers only the simulation itself --
bee movement, the polytunnel environment, foraging, and observation -- and
not the separate evolutionary optimisation layer (`PolyBeeEvolve`) that
searches over environment configurations using the simulation as its
fitness evaluator; that layer is described in the Experiments section.

## Overview {#sec:appendix-overview}

### Purpose

`polybee` simulates the foraging movement of a population of bees around
one or more hives, in an environment that may contain a rectangular
polytunnel (with a configurable number of entrances, optionally fitted
with anti-bird or anti-hail netting), linear barriers, and patches of
flowering plants. Its purpose is to generate, for a given fixed physical
configuration of these elements, the emergent spatial and temporal
patterns of bee activity -- where bees spend time, how successfully
flowers are pollinated, and how easily bees cross netted tunnel
boundaries -- that result from simple individual foraging, homing, and
obstacle-avoidance rules. The model is designed to serve as the
evaluation function for a separate *environment-shaping* optimisation
process (see @sec:environment-shaping-in-abms and the Experiments
section), which searches over candidate physical configurations to steer
these emergent patterns towards a target; this ODD description, however,
concerns only the single-configuration simulation dynamics themselves.

### Patterns

The model's outputs are used to assess environment designs against three
kinds of pattern:

- **Spatial coverage.** The heatmap of bee visitation (@sec:appendix-observation)
  should concentrate around plant patches and along accessible tunnel
  entrances/corridors, and should be sensitive in a plausible way to hive
  placement, entrance placement, and barrier placement.
- **Net-crossing behaviour.** Where tunnel entrances are fitted with
  anti-bird or anti-hail netting, the model's per-attempt exit
  probabilities and maximum-attempt counts (`net-antibird-exit-prob`,
  `net-antihail-exit-prob`, and associated max-attempts parameters) were
  calibrated so that the resulting simulated exit-success rate and mean
  number of rebounds reproduce the values reported for real netted
  enclosures in @sonter2024 (see `PARAM-NOTES.md` for the derivation).
  Reproducing those two summary statistics was used as a pattern-matching
  criterion when setting these parameters.
- **Pollination success.** The fraction of flowers receiving a "successful"
  number of visits (between `min-visit-count-success` and
  `max-visit-count-success`) should respond sensibly to changes in
  accessibility -- e.g. flowers cut off from a hive by barriers or by an
  unfavourable tunnel-entrance configuration should be visited less.

## Entities, state variables, and scales {#sec:appendix-entities}

**Bee** (agent). State: position `m_pos` (continuous $(x,y)$, environment
coordinates) and previous-step position `m_prevPos`; heading `m_angle`
(radians); energy `m_energy`; a purely cosmetic colour hue
`m_colorHue`; whether currently inside the tunnel `m_inTunnel`; a
behavioural state `m_state`, one of `FORAGING`, `ON_FLOWER`,
`RETURN_TO_HIVE_INSIDE_TUNNEL`, `RETURN_TO_HIVE_OUTSIDE_TUNNEL`,
`IN_HIVE`; step counters for the current foraging bout, hive stay, and
flower stay (`m_currentBoutDuration`, `m_currentHiveDuration`,
`m_currentFlowerDuration`); a bounded queue of waypoints for the current
return trip (`m_homingWaypoints`); a short rolling memory of recently
visited plants (`m_recentlyVisitedPlants`, length
`bee-visit-memory-length`); a bounded path history for observation/
rendering (`m_path`, length `bee-path-record-len`); transient state for an
in-progress tunnel-entrance crossing attempt (`m_tryingToCrossEntrance`,
`TryingToCrossEntranceState`); and records of past crossing attempts used
only for summary statistics (`m_entranceCrossingRecords`). Each bee holds
a reference to its home `Hive` and to the `Environment`.

**Hive.** State (fixed at construction): position `m_pos`; an exit/entry
direction `m_direction` (0=North, 1=East, 2=South, 3=West, 4=Random),
which sets the heading a bee adopts each time it arrives at or departs
the hive; and `m_inTunnel`, computed once from position.

**Plant** (flower). State: fixed position $(x,y)$; a `speciesID` used only
as a label/grouping tag (it plays no role in the interaction logic); a
visit count `m_visitCount`, incremented each time a bee lands; and a
nectar store `m_nectarAmount`, initialised to `flower-initial-nectar` and
depleted (never replenished) as bees extract it.

**Tunnel.** A single fixed axis-aligned rectangle (`tunnel-x`,
`tunnel-y`, `tunnel-width`, `tunnel-height`), represented as four boundary
line segments plus their unit and outward-normal vectors. It owns a set
of **tunnel entrances** (`TunnelEntranceInfo`): each has a fixed position
(a sub-span of one of the four sides, derived from a relative
`tunnel-entrance` spec), a side (N/E/S/W), and a net type (`NONE`,
`ANTIBIRD`, `ANTIHAIL`) which determines a fixed per-attempt exit
probability and maximum-attempt count (looked up from the corresponding
global `net-*` parameters, not stored per-entrance).

**Barrier.** A fixed line segment with no other state, used purely
geometrically to block or partially block bee movement.

**Environment.** The spatial container: fixed extent (`env-width` x
`env-height`), the `Tunnel`, the vectors of all `Bee`, `Hive`, `Plant`,
and `Barrier` objects, and two background spatial-index grids (not
visible to agents) used to accelerate nearby-plant and nearby-barrier
lookups.

**Heatmap and Flowmap** (observation only, not agents; see
@sec:appendix-observation) -- passive grids over the environment that accumulate
bee-position counts and bee-movement-direction statistics respectively.
Neither is read by any agent; they exist purely to record output.

**Spatial scale.** Positions and distances are continuous floating-point
values in a generic "environment coordinate" unit; the origin is
top-left, $x$ increases rightward and $y$ increases downward (so, for
example, a hive direction of "North" corresponds to heading angle
$-\pi/2$). The unit is not tied to an explicit real-world measure (e.g.
metres); it is calibrated indirectly and loosely through parameters such
as `bee-step-length`, `bee-visual-range`, and plant/tunnel spacing. The
example configuration in `polybee.cfg` uses a $600 \times 800$
environment containing a $400 \times 600$ tunnel. Discretisation into
grid cells occurs only in derived structures used for efficient lookup or
output aggregation (the plant and barrier spatial-index grids, and the
heatmap/flowmap) -- movement dynamics themselves take place in continuous
space.

**Temporal scale.** Time advances in discrete iterations; one iteration
is one synchronous update of every bee plus environment bookkeeping
(@sec:appendix-process-overview). A run lasts `num-iterations` iterations (default
100; the example `polybee.cfg` configuration uses 2000). There is no
explicit mapping from one iteration to a real-world duration such as
seconds; `bee-step-length` and the energy-depletion-per-step parameter
are the effective calibration knobs for how far/how costly one iteration
of movement is.

## Process overview and scheduling {#sec:appendix-process-overview}

**Setup (once per run)**, performed by `Environment::initialise()`: build
the `Tunnel` and its entrances; build `Barrier`s (each `barrier` spec may
be replicated into a grid of individual barriers via repeat-count/spacing
parameters) and index them spatially; build `Plant`s (each `patch` spec
laid out as a regular sub-grid of plants at the given spacing, each then
perturbed by independent Gaussian jitter) and index them spatially; build
`Hive`s and, divided evenly across them, `numBees` `Bee`s; and initialise
the (empty) Heatmap and Flowmap.

**Each iteration**, `PolyBeeCore::run()` calls `Environment::update()`,
which:

1. Updates every `Bee` in turn (in a fixed, arbitrary order -- the vector
   order in which they were created), each bee's `Bee::update()`
   dispatching on its current state to exactly one of: `forage()`,
   `stayOnFlower()`, `returnToHiveInsideTunnel()`,
   `returnToHiveOutsideTunnel()`, or `stayInHive()` (submodels below).
   Because bees never sense or interact with one another directly
   (@sec:appendix-interaction), this update order has no effect on the dynamics --
   only, potentially, on rendering order.
2. Updates the Heatmap (records each bee's current cell).
3. If `flowmap-update-period` > 0 and the current iteration is a multiple
   of it, updates the Flowmap (records each bee's movement vector for
   this step, skipped for bees that did not move, e.g. those `ON_FLOWER`
   or `IN_HIVE`).
4. If visualisation is enabled, redraws the current frame (a configurable
   delay, `vis-delay-per-step`, paces the real-time display but does not
   affect the logical step count).

The run stops when the iteration counter reaches `num-iterations`, or on
an early-exit request (e.g. the user closes the visualisation window).
At the end of a run, if logging is enabled, the Flowmap's per-cell
statistics are finalised and the heatmap, flowmap, effective
configuration, and a run-info summary are written to file
(@sec:appendix-observation).

## Design concepts {#sec:appendix-design-concepts}

### Basic principles {#sec:appendix-basic-principles}

Bees follow a correlated random walk biased towards nearby, unvisited flowers --
a nearest-flower-seeking heuristic rather than a true Lévy flight. An energy
budget accumulated from nectar and depleted per step drives the length of a
foraging bout and the decision to return to the hive. Tunnel-boundary
permeability is represented stochastically, with per-attempt exit probabilities
calibrated directly from published field-trial statistics on real
anti-bird/anti-hail netting reported in @sonter2024 (see `PARAM-NOTES.md`),
rather than from a first-principles physical model of the netting itself.

### Emergence {#sec:appendix-emergence}

The population-level patterns of interest -- the visitation heatmap, the
per-plant visit counts and the resulting successful-visit fraction, and
the movement flow field -- are not encoded directly anywhere in the
model. They emerge purely from many bees repeatedly applying the same
local rules (move towards a sensed unvisited flower with fixed
probability, else take a constrained random turn; bounce, rebound, or
pass at tunnel walls/entrances/barriers; return to hive when energy
leaves a fixed band) within one particular fixed spatial arrangement of
hive(s), tunnel, entrances, barriers, and plant patches.

### Adaptation

None. Bees do not modify their decision rules based on experience within
a run. Their only form of memory -- the short rolling list of recently
visited plants (@sec:appendix-sensing) -- exists to avoid immediate re-visits, not
to adapt behaviour.

### Objectives

Bees do not evaluate an explicit objective or utility function. The
energy state variable creates an implicit approach/avoidance dynamic
(deplete while foraging, replenish from nectar, return to hive once
energy leaves the $[\text{bee-energy-min-threshold},
\text{bee-energy-max-threshold}]$ band) but this is a fixed threshold
rule, not an optimisation performed by the agent.

### Learning

None.

### Prediction

None. Every movement decision uses only the bee's current position/state
and what it currently senses; there is no internal model of, or
anticipation of, future states.

### Sensing {#sec:appendix-sensing}

A bee senses two things, both purely local and both computed via
background spatial-index grids restricted to a $3\times3$ block of cells
around the bee (cell size $\approx$ `bee-visual-range` for plants,
$\approx$ the longer of the largest barrier length or
`bee-visual-range` for barriers):

- **Plants**: all plants within `bee-visual-range` of the bee's current
  position, excluding any in its rolling recently-visited memory
  (`bee-visit-memory-length` entries), and excluding any whose direct
  line of sight is obstructed by a tunnel wall or a barrier.
- **Barriers**: any barrier that would obstruct the straight-line segment
  of a candidate random-walk step, used only to modify that step
  (@sec:appendix-submodel-movement).

Bees do not sense other bees, the hive from a distance (beyond having a
fixed internal route home), or any aggregate/global state -- the
Heatmap and Flowmap are write-only observation structures and are never
read by agents.

### Interaction {#sec:appendix-interaction}

Bees do not interact with one another directly: there is no collision
avoidance, communication, or recruitment behaviour between bees. The only
channel by which one bee's actions affect another is indirect, through
shared `Plant` state -- one bee extracting nectar or incrementing a
plant's visit count permanently changes what the next bee to visit that
plant will experience. Bees do interact with static environment entities:
tunnel walls and entrances (bounce, rebound, or pass, depending on net
type), barriers (path blocked, flown over, or shortened), plants (visit,
extract nectar, increment visit count), and the hive (fixed return
destination and heading reference).

### Stochasticity

Every stochastic draw in the model comes from a single shared
`std::mt19937` engine, seeded once per run (or once per island, when run
under the separate optimisation layer) from an alphanumeric seed string
(`rng-seed`, or a randomly generated one if not supplied, which is then
recorded back into the logged configuration for reproducibility).
Randomness enters at:

- a bee's cosmetic trail colour (uniform);
- initial/hive-exit heading, when a hive's direction is `4` (Random):
  drawn uniform on $[0, 2\pi)$; otherwise heading is fixed to the
  hive's compass direction;
- the per-step turning noise added during a random-walk step (uniform on
  $[-\delta_{\max}, \delta_{\max}]$, @sec:appendix-submodel-movement);
- the choice, when an unvisited flower is in range, between heading for
  it and taking a random-walk step instead (Bernoulli,
  `bee-prob-visit-nearest-flower`);
- selection among multiple simultaneously visible unvisited flowers
  (distance-weighted random choice, closer flowers more likely);
- tunnel-entrance crossing success, both while foraging and while homing
  (independent Bernoulli trials at the net type's per-attempt exit
  probability, on every attempt);
- flying over a barrier rather than being blocked by it (Bernoulli,
  `barrier-pass-prob`);
- positional jitter added to each step while following a homing waypoint,
  except the final approach step (Gaussian, mean 0, s.d. $0.1\times$ the
  step length);
- plant position jitter at initialisation (Gaussian, s.d. = the patch's
  jitter parameter).

### Collectives {#sec:appendix-collectives}

None. Bees are not organised into any super-agent structure such as a
swarm or division of labour. A hive is a shared spawn/return point and
orientation reference, not a collective with its own dynamics; the bee
population is simply divided evenly across the configured hives
(`num-bees` / number of hives, with a warning if it does not divide
evenly).

### Observation {#sec:appendix-observation}

Two passive spatial accumulators record model output and are never read
by agents: the **Heatmap** (bee-visitation counts per cell, raw and
normalised, cell size `heatmap-cell-size`) and the **Flowmap** (per-cell
predominant movement axis and alignment strength, computed via a
double-angle circular mean of recorded movement headings, plus an
observation count; cell size `flowmap-cell-size`, updated only every
`flowmap-update-period` iterations). Per-bee path history and
tunnel-crossing attempt records are also retained, the latter only for
crossing attempts made while `FORAGING` (see the note under Submodels) --
these feed a run-end summary of crossing success rate and mean rebounds.
At the end of a logged run, files are written for: the raw and normalised
heatmap, the flowmap, the effective configuration, and a human-readable
run-info summary (crossing statistics, successful-visit fraction, and,
if a target heatmap was configured, the earth mover's distance to it --
a metric used actively only by the separate optimisation layer). If
visualisation is enabled, bees, trails, the tunnel, barriers, patches,
and the heatmap/flowmap can be viewed live, with interactive controls to
pause, change display mode, and toggle overlays.

## Details {#sec:appendix-details}

### Initialization {#sec:appendix-initialization}

Parameters are read from a config file (default `polybee.cfg`) and then
overridden by any command-line arguments given (command line wins on
conflict); derived parameters are computed and a consistency check is
run (e.g. requiring at least one hive, clamping invalid visualisation
settings, disabling path recording when not visualising). The RNG is
seeded once, from `rng-seed` if supplied, else from a freshly generated
random alphanumeric string (which is then recorded back into the
effective configuration for reproducibility). The environment is then
built in this order: tunnel and its entrances (fixed position/size, each
entrance a fixed relative span on a given side with a given net type);
barriers (each `barrier` spec possibly repeated into a grid of
individual segments); plants (each `patch` spec laid out as a regular
sub-grid at the given spacing, each individual position then perturbed
by independent Gaussian jitter, each plant seeded with
`flower-initial-nectar`); hives (fixed position and direction, one per
`hive` spec) and bees (`num-bees` divided evenly across hives, each bee
starting at its hive's position with initial energy
`bee-initial-energy`, empty path/memory, and a heading set from its
hive's direction); and the empty Heatmap and Flowmap. (When run under the
separate optimisation layer with hive positions themselves being
evolved, hive/bee creation at this point is skipped and handled instead
by that layer for each candidate configuration -- not relevant to a
plain simulation run, which is what this ODD describes.)

### Input data

The simulation does not consume any external time-varying driving data.
The only optional external file is a target-heatmap CSV
(`target-heatmap-filename`), read once at initialisation solely to
report the earth mover's distance to it in the run-info output; it does
not otherwise affect simulation dynamics (it is used actively as a
fitness signal only in the separate optimisation layer). The tunnel
net-crossing probability and max-attempt parameters, while fixed
constants at run time, were derived offline from published field-trial
exit-success and rebound-count statistics for real anti-bird and
anti-hail netting reported in @sonter2024 -- see `PARAM-NOTES.md` for
the derivation.

### Submodels

#### Movement/turning rule (random walk step) {#sec:appendix-submodel-movement}

When a bee finds no unvisited flower in range, or (with probability
$1-p_{\text{nearest}}$, `bee-prob-visit-nearest-flower`) chooses not to
head for one it has found, it takes a turning-rate-bounded random-walk
step:
$$
\theta_{t+1} = \theta_t + U(-\delta_{\max}, \delta_{\max}), \qquad
x_{t+1} = x_t + \ell \cos\theta_{t+1}, \qquad
y_{t+1} = y_t + \ell \sin\theta_{t+1}
$$
where $\delta_{\max}$ is `bee-max-dir-delta`, $\ell$ is
`bee-step-length`, and $U(a,b)$ is a uniform draw. Before this move is
accepted, it is checked against nearby barriers
(`Environment::distanceToNearestObstructingBarrier`): if the path is
obstructed, the bee either flies over the barrier anyway (Bernoulli,
`barrier-pass-prob`), or the step is shortened to 90% of the distance to
the barrier (if that is above a small minimum), or, if too close to
shorten further, a new random direction is tried (up to 5 attempts before
the bee simply stays put for that iteration).

**Directed movement towards a sensed flower.** If an unvisited flower is
within `bee-visual-range` (@sec:appendix-sensing) and is chosen (probability
$p_{\text{nearest}}$), the bee's heading is set directly to the bearing
to that flower -- $\theta_{t+1} = \operatorname{arctan2}(y_f - y_t,\, x_f -
x_t)$ -- **not** bounded by $\delta_{\max}$; this is the one situation in
which a bee's turn is unconstrained. If the flower is within one step
length it moves directly onto it (triggering a landing, below); otherwise
it takes one step of length $\ell$ towards it.

**Environment-boundary handling.** If a candidate move would cross the
outer edge of the environment, the position is clamped to the edge and
the heading re-aligned to run along that edge (the bee slides along the
boundary rather than reflecting off it).

**Tunnel-boundary crossing while foraging.** If a candidate foraging move
crosses the tunnel boundary at an entrance, a single Bernoulli trial at
that entrance's per-attempt exit probability ($1.0$ for `NONE`,
`net-antibird-exit-prob` $\approx 0.119$ for `ANTIBIRD`,
`net-antihail-exit-prob` $\approx 0.037$ for `ANTIHAIL`) determines
success; on success the bee crosses and its `m_inTunnel` flag flips. On
failure the bee enters a bounded rebound sub-state: it alternates between
a small random side-step-then-rebound position (perpendicular distance
back from the net) and a fresh approach attempt towards the net, up to
$2\times$ the net type's max-attempts parameter (`
net-antibird-max-exit-attempts` = 7, i.e. 14 moves;
`net-antihail-max-exit-attempts` = 11, i.e. 22 moves) before giving up
and reverting to normal foraging at its current position. If a candidate
move instead hits solid tunnel wall (not an entrance), the bee's position
is clamped to the wall and its heading re-aligned along the wall, as at
the outer boundary. A small fixed buffer (0.1 units) is maintained
between bees and tunnel walls at all times to avoid floating-point edge
cases.

**Flower visitation and nectar extraction.** On landing on a targeted
flower, the plant's visit count is incremented, the plant is added to the
bee's recently-visited memory, the bee's state becomes `ON_FLOWER`, and
its energy increases by `Plant::extractNectar(bee-energy-boost-per-flower)`
-- capped by however much nectar the plant has left (nectar is never
replenished). The bee remains `ON_FLOWER` for `bee-on-flower-duration`
iterations before resuming foraging (its foraging-bout duration counter
is *not* reset by a flower visit, only by a full return-to-hive cycle).

**Energy dynamics and the decision to return to hive.** While foraging
(and not mid-landing), energy depletes by `bee-energy-depletion-per-step`
each iteration; if it falls to or below
`bee-energy-min-threshold`, or rises to or above
`bee-energy-max-threshold`, the bee switches to returning to the hive.
Energy is reset to `bee-initial-energy` only once the bee has completed a
full stay in the hive (`bee-in-hive-duration` iterations) and resumes
foraging.

**Return-to-hive waypoint routing.** On switching to return-to-hive, the
bee's recently-visited-plant memory is cleared (so the next bout can
revisit the same flowers) and a route is computed once: if the bee is
currently inside the tunnel, its route is a single waypoint -- straight
to the hive if the hive is also inside the tunnel (no barrier avoidance
is applied to this leg, unlike foraging movement), or to a point just
outside the last tunnel entrance used, if the hive is outside. If the bee
is currently outside the tunnel, and a straight line to its endpoint
(hive, or the last-used entrance if the hive is inside) does not cross
the tunnel's bounding rectangle, that direct line is the route; otherwise
up to eight candidate routes via the tunnel's four corners (single-corner
and adjacent-corner-pair options, each corner offset outward by the wall
buffer) are evaluated, tunnel-intersecting candidates discarded, and the
shortest valid one chosen (falling back to a direct, unchecked line to
the endpoint in the rare case that no candidate avoids the tunnel, a
known edge case in the current implementation).

**Waypoint following while homing.** Each iteration, the bee moves
directly towards its current waypoint at full step length (or the
remaining distance, if shorter), with independent Gaussian positional
jitter (mean 0, s.d. $0.1\times$ the step length) added unless this step
reaches the waypoint. If the waypoint reached is a tunnel entrance (only
possible as the final waypoint, when the bee's in/out-of-tunnel status
differs from its hive's), passing it requires an additional Bernoulli
success at that entrance's per-attempt exit probability, retried
independently every iteration with **no** attempt limit and **no**
rebound offset -- unlike the bounded, offset-rebound sub-model used while
foraging (above). Homing-mode crossing attempts are also *not* included
in the entrance-crossing success/rebound statistics reported at the end
of a run, which cover foraging-mode attempts only.

**Heatmap and flowmap recording.** Each iteration, every bee's current
cell (at `heatmap-cell-size` resolution) has its visitation count
incremented. Every `flowmap-update-period` iterations, every bee that
moved this step has its movement direction $\theta = \operatorname{arctan2}
(\Delta y, \Delta x)$ recorded into its current cell (at
`flowmap-cell-size` resolution); at the end of a run the predominant
(headless) axis and alignment strength for each flowmap cell are computed
from the circular mean of $2\theta$ across all recorded movements in that
cell.

### Parameter reference

@tbl:appendix-params lists the simulation-relevant parameters, grouped as in
`polybee.cfg`; parameters that control only the separate optimisation
layer (`evolve`, `evolve-objective`, `evolve-spec`,
`target-heatmap-filename`'s active use as a fitness target,
`num-configs-per-gen`, `num-trials-per-config`, `num-generations`,
`num-islands`, `migration-*`, `use-diverse-algorithms`,
`bridge-overlaps-allowed`) are omitted.

| Parameter | Meaning | Default |
|---|---|---|
| `rng-seed` | RNG seed string (empty/`0` = random) | *(random)* |
| `num-​iterations` | Number of simulation iterations | 100 |
| `env-width`, `env-height` | Environment extent | 450, 250 |
| `tunnel-x`, `tunnel-y` | Top-left position of tunnel | 200, 100 |
| `tunnel-width`, `tunnel-height` | Tunnel extent | 50, 50 |
| `tunnel-​entrance` | Entrance span, side, net type (repeatable) | *(none)* |
| `net-​antibird-​exit-​prob` | Per-attempt exit probability, anti-bird net | 0.1187 |
| `net-​antihail-​exit-​prob` | Per-attempt exit probability, anti-hail net | 0.0371 |
| `net-​antibird-​max-​exit-​attempts` | Max exit attempts, anti-bird net | 7 |
| `net-​antihail-​max-​exit-​attempts` | Max exit attempts, anti-hail net | 11 |
| `barrier` | Barrier line, with optional repeat grid (repeatable) | *(none)* |
| `barrier-​pass-​prob` | Probability of flying over a barrier | 0.0 |
| `patch` | Flower patch spec (repeatable) | *(none)* |
| `plant-​default-​spacing` | Default plant spacing for evolved bridge patches | 10.0 |
| `plant-​default-​jitter` | Default plant jitter for evolved bridge patches | 0.1 |
| `flower-​initial-​nectar` | Initial nectar per flower | 100.0 |
| `min-​visit-​count-​success` | Lower bound of "successful" visit count | 1 |
| `max-​visit-​count-​success` | Upper bound of "successful" visit count | 1000 |
| `num-bees` | Number of bees | 50 |
| `bee-​max-​dir-​delta` ($\delta_{\max}$) | Max heading change per random-walk step (radians) | 0.4 |
| `bee-​step-​length` ($\ell$) | Distance moved per step | 20.0 |
| `bee-​path-​record-​len` | Max positions retained in a bee's path history | 250 |
| `bee-​visual-​range` | Max distance at which a bee can detect a flower | 1.0 |
| `bee-​visit-​memory-​length` | Recently visited plants remembered | 5 |
| `bee-​prob-​visit-​nearest-​flower` ($p_{\text{nearest}}$) | Probability of heading for a sensed flower vs. random walk | 0.9 |
| `bee-​in-​hive-​duration` | Iterations spent in hive between bouts | 200 |
| `bee-​initial-​energy` | Energy on leaving the hive | 100.0 |
| `bee-​energy-​depletion-​per-​step` | Energy cost per foraging step | 1.0 |
| `bee-​energy-​boost-​per-​flower` | Energy gained per flower visit (nectar-limited) | 10.0 |
| `bee-​on-​flower-​duration` | Iterations spent on a flower | 5 |
| `bee-​energy-​min-​threshold` | Energy floor triggering return to hive | 0.0 |
| `bee-​energy-​max-​threshold` | Energy ceiling triggering return to hive | 100.0 |
| `hive` | Hive position and direction (repeatable) | *(none, >=1 required)* |
| `heatmap-​cell-​size` | Heatmap cell size | 10 |
| `flowmap-​cell-​size` | Flowmap cell size | 10 |
| `flowmap-​update-​period` | Iterations between flowmap updates (0 = never) | 1 |
| `logging` | Write output files at end of run | true |
| `log-dir` | Output directory | `.` |
| `log-​filename-​prefix` | Output filename prefix | *(none)* |
| `visualise` | Show real-time graphical display | true |
| `vis-​cell-​size` | Display size of one environment unit | 1.0 |
| `vis-​delay-​per-​step` | Delay per step when visualising (ms) | 100 |
| `vis-​bee-​path-​draw-​len` | Max path segments drawn per bee | 250 |

: Simulation-relevant parameters, with defaults from the `Params`
registry. The example configuration in `polybee.cfg` overrides several
of these (see the Experiments section). {#tbl:appendix-params}

See @tbl:appendix-params for the full parameter set used in the experiments below.

# Appendix B: Miscellaneous Technical Notes {#sec:appendix-misc-tech-notes}

When running the experiments reported in this paper, a target heatmap was also
specified in the configuration files, but since we were using the visit-fraction
criterion (rather than distance-to-target) as the active fitness objective for
these experiments, the target heatmap played no role in the search itself.

The `polybee` package provides the facility of running "island model" genetic
algorithms, where total populations are split over multiple islands with
occasional migration between them. However, for the experiments reported here,
the search ran as a single island (`num-islands=1`), so no migration took place
despite migration parameters being set.

# Appendix C: Additional figures from analysis of results {#sec:appendix-additional-figures}

## Evolution of fitness over generations

## Flowfield analysis of bee movements under each condition


::: {#fig:bee-flowmap-size-25-all-conditions}
![Baseline (no barriers or bridges)](figures/bee-flowmap-size-25-intra-condition-merged-baseline-runs-2000its.png){#fig:bee-flowmap-sz25-baseline width=45%}
![Evolved 10 bridges only](figures/bee-flowmap-size-25-intra-condition-merged-evolve-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz25-10B width=45%}

```{=latex}
\mbox{}\\
```

![Evolved 20 barriers only](figures/bee-flowmap-size-25-intra-condition-merged-evolve-20X-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz25-20X width=45%}
![Evolved 20 barriers and 10 bridges](figures/bee-flowmap-size-25-intra-condition-merged-evolve-20X-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz25-20X10B width=45%}

Flowmaps showing predominant directions of bee movement, recorded at the resolution of 25x25 unit cells.
:::


::: {#fig:bee-flowmap-size-50-all-conditions}
![Baseline (no barriers or bridges)](figures/bee-flowmap-size-50-intra-condition-merged-baseline-runs-2000its.png){#fig:bee-flowmap-sz50-baseline width=45%}
![Evolved 10 bridges only](figures/bee-flowmap-size-50-intra-condition-merged-evolve-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz50-10B width=45%}

```{=latex}
\mbox{}\\
```

![Evolved 20 barriers only](figures/bee-flowmap-size-50-intra-condition-merged-evolve-20X-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz50-20X width=45%}
![Evolved 20 barriers and 10 bridges](figures/bee-flowmap-size-50-intra-condition-merged-evolve-20X-10B-400gen-400pop-100epi-2000its.png){#fig:bee-flowmap-sz50-20X10B width=45%}

Flowmaps showing predominant directions of bee movement, recorded at the resolution of 50x50 unit cells.
:::


::: {#fig:angdelta-heatmap-size-50-all-conditions}
![Baseline bee movements (no barriers or bridges)](figures/bee-flowmap-size-10-intra-condition-merged-baseline-runs-2000its.png){#fig:bee-flowmap-sz10-baseline3 width=45%}
![Angular deltas for Evolved 10 bridges only](figures/angdelta-heatmap-sz50-nothresh-cross-analysis-evolve-10B-vs-baseline.png){#fig:angdelta-heatmap-sz50-10B width=45%}

```{=latex}
\mbox{}\\
```

![Angular deltas for Evolved 20 barriers only](figures/angdelta-heatmap-sz50-nothresh-cross-analysis-evolve-20X-vs-baseline.png){#fig:angdelta-heatmap-sz50-20X width=45%}
![Angular deltas for Evolved 20 barriers and 10 bridges](figures/angdelta-heatmap-sz50-nothresh-cross-analysis-evolve-20X-10B-vs-baseline.png){#fig:angdelta-heatmap-sz50-20X-10B width=45%}

Analysis of change in bee movements caused by barriers and bridges, recorded at the resolution of 50x50 units cells.
:::

## Analysis of change in bee movements caused by barriers and bridges


# Appendix D: How Bee-Movement Flowmaps Are Generated and Merged {#sec:appendix-flowmap-merging}

Each flowmap shown in the main text and in @sec:appendix-additional-figures
is a merge of many independent per-replicate flowmaps (50 for each evolved
condition, 5000 for the baseline), produced by `tools/run_analysis.sh` from
each replicate's own best-performing configuration.

**Where the per-replicate flowmaps come from.** During the original
evolutionary search, every trial simulation is run without logging any
output files, so no flowmap is ever saved for any individual configuration
evaluated during evolution itself. `run_analysis.sh` therefore performs a
fresh, single re-run of each replicate's champion (best-individual)
configuration -- once per flowmap cell size (10, 25, and 50) -- purely to
generate loggable flowmap and heatmap output for analysis. This is why the
analysis pipeline needs an explicit re-run step at all, rather than simply
reusing output from the evolutionary search.

**What a single replicate's flowmap contains.** Each simulation step, for every
bee that moved, its raw movement heading $\theta = \operatorname{arctan2}(\Delta
y, \Delta x)$ is recorded into whichever cell the bee currently occupies. At the
end of the run, each cell's list of recorded headings is reduced to a single
`axis:strength:count` value using a double-angle circular mean: the heading
values are doubled, their sine and cosine components summed and averaged, and
the predominant (headless) axis and the strength of alignment to it are
recovered from that average vector: `axis`
$=\tfrac{1}{2}\operatorname{arctan2}(\overline{\sin 2\theta},\overline{\cos
2\theta})$, `strength` $=|\overline{\sin 2\theta},\overline{\cos 2\theta}|$ (0
when recorded movements in that cell point in unrelated directions, 1 when they
were all exactly aligned to `axis`); `count` is simply the number of individual
movement observations recorded in that cell.
So `strength` is a mean resultant length (a standard circular-statistics
concentration measure, in [0, 1]) of the doubled bee headings in that cell:
a `stength` value near 1.0 means nearly all bee movements through that cell were aligned
along (or exactly opposite) the same axis --- a strong, coherent flow direction;
`strength` of 0.0 means movements in that cell were scattered across many different
directions with no dominant axis --- even if count is high, there's no
consistent flow to report. `strength` is independent of `count` (which just
tracks how many movements were observed): a cell can have a high `count` but low
`strength` if bees passed through it moving every which way, or a low `count`
but `strength` of exactly 1.0 if the handful of bees that did pass through all
moved along the same line.

**How the per-replicate flowmaps are merged.** Merging of flowmaps (performed by the
`merge_flowmaps.py` script called by `run_analysis.sh`) does not simply
average each replicate's `axis`/`strength` values cell by cell; doing so
would treat every replicate as equally informative regardless of how many
movements it actually contributed to a cell, and naively averaging a
circular quantity like `axis` is not mathematically well defined in
general. Instead, for each cell, the merge step first reconstructs each
replicate's underlying weighted sine/cosine sums (multiplying each
replicate's own `count`, `strength`, and doubled `axis` back together),
pools those sums across all contributing replicates, and only then
recomputes a single merged `axis` and `strength` from the pooled totals,
with merged `count` equal to the sum of all replicates' counts. This is
mathematically equivalent to pooling every individual bee-movement
observation from every replicate run into one single per-cell circular-mean
calculation, so replicates that recorded more movements in a given cell
correctly contribute proportionally more to that cell's merged value.

The merged bee-position heatmaps shown alongside these flowmaps are
produced by the same "one fresh re-run per replicate, then combined" scheme,
but merging there is simpler: each replicate's heatmap cell values are
already normalised fractions (summing to 1.0 across the whole grid), so the
merged heatmap is just their mean, cell by cell, across all replicates (see
@sec:effect-of-barriers-and-bridges-on-bee-coverage-of-the-environment).


# Appendix E: How Angular-Delta Heatmaps Are Generated {#sec:appendix-angdelta}

The angular-delta ("angdelta") heatmaps shown in
@sec:analysis-of-change-in-bee-movements-caused-by-barriers-and-bridges are
produced by `tools/gen_angdelta_data.py`, run once per pair of merged
flowmaps being compared (@sec:appendix-flowmap-merging), for each
combination of thresholds used in @sec:setup-cross-condition-analysis.

**The angular-delta value itself.** For each cell, given the two flowmaps'
predominant axes $\alpha_1$ and $\alpha_2$ at that cell (see
@sec:appendix-flowmap-merging for how a flowmap's per-cell axis is
computed), the angular delta is:
$$
\Delta = \tfrac{1}{2}\operatorname{arccos}\bigl(\cos(2(\alpha_1 - \alpha_2))\bigr)
$$
As with the flowmap axes themselves, this treats each axis as headless (a
direction and its exact opposite are equivalent), and yields a delta in
$[0, \pi/2]$: 0 where the two flowmaps' predominant directions in that cell
coincide (or are exactly opposed), up to $\pi/2$ (90 degrees) where they are
perpendicular.

**Excluding cells without enough data.** A cell is excluded from the heatmap
(scored 0, rather than an actual computed delta) if either flowmap has no
recorded movements there at all (strength $\le 0$ or count $=0$), matching the
same hard exclusion rule used when drawing flowmaps directly
(@sec:appendix-flowmap-merging). Beyond that, three optional thresholds can
exclude further cells:

- `--strength-th`: matching the equivalent thresholding available when
  visualising a flowmap on its own, at least one of the two flowmaps'
  entries for that cell must have alignment strength at or above this
  value; if both are below it, the cell is excluded.
- `--count-th`: also matching flowmap thresholding, expresses each
  flowmap's own count for that cell as a fraction of the largest count
  found anywhere in that same flowmap, and both flowmaps' fractions for
  the cell must be at or above this value; if either falls short, the cell
  is excluded.
- `--delta-th`: unlike the two thresholds above, this is applied to the
  *computed angular delta itself* (in radians), not to either flowmap's
  strength or count, and only once a cell has already passed the
  `--strength-th` / `--count-th` checks. If the resulting delta $\Delta$
  is below `--delta-th`, the cell is excluded -- this filters out cells
  where the two conditions' predominant directions are too close together
  to be a meaningfully different axis, as opposed to cells where the
  underlying flowmap data is simply too weak or sparse to trust.

**Marking excluded cells.** By default an excluded cell (for any of the
reasons above) is written into the heatmap CSV as a plain 0, which is
indistinguishable from a cell whose *computed* delta genuinely happens to be
0 (i.e. the two conditions' predominant directions coincide exactly). Passing
`--x-below-th` to `gen_angdelta_data.py` instead writes an `x` marker for any
excluded cell, and passing the same flag to `visualize_heatmap.py` when
plotting the resulting CSV renders those `x`-marked cells as plain white,
rather than colouring them using the heatmap's normal colour scale -- so a
white cell in the figures means "excluded", not "zero delta".

# Appendix F: Bootstrap Significance Testing of Median Fitness Differences {#sec:appendix-bootstrap}

The significance tests reported in @sec:overallstats
(@tbl:fitness-significance) are produced by `bootstrap_median_test.py`, run
from `calc_fitness_stats.sh` alongside the min/median/IQR summary in
@tbl:fitness-summary. For each evolved condition it compares that
condition's 50 champion-fitness values against the baseline's 5000 per-run
values (the same `champion-fitnesses-*.csv` files `calc_fitness_stats.sh`
reads).

**Why a bootstrap, and why the median.** The baseline and evolved conditions
have very different sample sizes (5000 vs. 50), and the fitness metric is a
bounded, non-normally-distributed proportion-like quantity, which is also why
@tbl:fitness-summary reports median/IQR rather than mean/SD in the first
place. A parametric test such as a t-test would assume near-normal sampling
distributions for the *mean*; a nonparametric bootstrap instead targets the
median directly, requires no distributional assumptions, and handles the
50-vs-5000 mismatch without any special adjustment: each sample is simply
resampled (with replacement) at its own size, independently of the other, so
each group is treated as its own random sample from its own population.

**Confidence interval.** For each evolved condition, both samples are
resampled independently 10,000 times (`--n-resamples`, default 10000), and
the difference $\text{median(condition)} - \text{median(baseline)}$ is
recomputed on each pair of resamples, giving a bootstrap distribution of the
median difference. A 95% confidence interval is taken from this distribution
using the BCa (bias-corrected and accelerated) method, which is generally
more accurate than a plain percentile interval for a skewed bootstrap
distribution. BCa's acceleration constant is itself estimated via a
jackknife on the median -- a non-smooth statistic -- which can make that
estimate degenerate (a zero denominator) for some samples; when this
happened (for the bridges-only condition), the script automatically falls
back to the plain percentile bootstrap CI for that condition and reports
which method was used (`ci_method` column in its output).

**p-value.** A two-sided bootstrap hypothesis-test p-value is computed
separately from the confidence interval, using the "shift" method of Davison
& Hinkley (*Bootstrap Methods and Their Application*, 1997, ch. 4): both
samples are first recentred by subtracting their own median, so that the
null hypothesis of equal medians holds by construction; 10,000 resamples are
then drawn independently from these recentred samples, and the p-value is
the fraction of resulting null differences at least as extreme (in absolute
value) as the difference actually observed in the unshifted data, with +1
smoothing added to numerator and denominator to avoid ever reporting $p=0$
(so the smallest value the test can report at 10,000 resamples is
$1/10001\approx0.0001$). For all three evolved conditions here, none of the
10,000 null-shifted resamples were as extreme as the observed difference, so
each comparison hits this floor: $p_\text{raw}\approx0.0001$ should be read
as "$p<0.0001$" rather than as a precisely estimated probability: it would
take more resamples to resolve a smaller p-value than the floor, though this
makes no difference to the significance conclusion at the $\alpha=0.05$
threshold used here.

**Multiple comparisons.** Since all three evolved conditions are compared
against the same baseline sample, a Holm-Bonferroni correction is applied
across the three resulting p-values to control the family-wise error rate,
giving the `p_holm` values reported in @tbl:fitness-significance.

**Interpretation.** This test answers the question "does the median
champion-configuration performance under evolution differ from the median
single-run baseline performance?" -- not "would a randomly chosen
configuration under condition X outperform a random baseline run?" Each
evolved condition's 50 values are themselves the best individual found by a
genetic algorithm (400 generations $\times$ 400 configurations per
generation $\times$ 100 trials per configuration; @sec:evosearch), not raw
single-run outcomes like the baseline's, so the two samples are not draws
from directly comparable underlying processes -- only their champion-vs-
single-run medians are being compared here.

Usage: `python3 bootstrap_median_test.py [--n-resamples N] [--seed S]
[--alpha A]`, run from the same directory as `calc_fitness_stats.sh` (it
expects the same per-condition `fitness-data-agg/` layout). The RNG seed
defaults to a fixed value for reproducibility.

# References
