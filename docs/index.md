# PolyBee

PolyBee is an agent-based model of bee movement in polytunnels. It simulates
bees foraging for nectar in an environment that may contain a polytunnel
(with configurable entrances, and optional anti-bird/anti-hail netting),
barriers, plant patches, and one or more hives. Bees move, sense and visit
nearby flowers, accumulate and expend energy, and return to their hive to
rest between foraging bouts.

PolyBee can be run as a straightforward simulation with a fixed
configuration, or in an "evolve" mode that uses genetic optimization to
search for configurations (e.g. entrance positions, hive positions, bridge
placements, barrier placements) that best match a target pattern of bee
movement or maximise the fraction of flowers that receive a successful
number of visits.

Simulation output includes CSV heatmaps of where bees have been, CSV
flowmaps of the predominant direction of bee movement through each part of
the environment, and (in evolve mode) the best-performing configurations
found by the optimizer.

## Documentation

- [User Guide](user-guide.md) — how to configure and run PolyBee in normal
  and evolve modes, and the purpose and format of the files it produces.
- [Tools](tools.md) — overview of the analysis and visualisation scripts in
  `tools/`, and how to use them.

## Source

PolyBee is written in C++20 and uses Raylib for real-time visualisation and
Pagmo for the evolutionary optimization in evolve mode. Source code and
issue tracking live in the project's GitHub repository.
