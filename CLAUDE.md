# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PolyBee is an agent-based model of bee movement in polytunnels, written in C++20. The application simulates bee behavior in a defined environment with configurable parameters and real-time visualization using Raylib.

## Build System

The project uses CMake with Ninja generator and includes custom build scripts:

- **Build**: `./make-debug` (runs `cmake -S . -B build -G Ninja && cmake --build build`)
- **Run**: `./run-debug [args]` (executes `./build/bin/debug/polybee`)
- **Configuration**: Parameters loaded from `polybee.cfg` or passed via command line

Build targets:
- Debug build (default): `-g -O0`
- Release build: `-O3 -DNDEBUG`
- Output directories: `build/bin/debug/` or `build/bin/release/`

## Architecture

### Core Classes

- **PolyBeeCore**: Main simulation controller that manages the simulation loop, bee collection, and environment. Contains the RNG engine and handles initialization from command-line arguments and config file.

- **Bee**: Individual bee agent with position (x,y), movement angle, path tracking, and movement behavior. Each bee has a configurable maximum direction change per step and records its path for visualization.

- **Environment**: Manages the simulation environment and handles collision detection. Currently basic but designed to be extended with walls, obstacles, and hive locations.

- **Params**: Flexible parameter system using a registry pattern. Parameters can be set via config file (`polybee.cfg`) or command line. Includes simulation control, environment dimensions, bee behavior, hive specifications, and visualization settings.

- **LocalVis**: Raylib-based visualization system that renders bees, their paths, environment boundaries, and simulation status in real-time.

### Key Design Patterns

- Static parameter registry in `Params` class - to add new parameters: add static variable, instantiate in Params.cpp, add to registry in `initRegistry()`
- Factory pattern for bee creation with configurable starting positions and angles
- Path recording system in bees for visualization and analysis
- Hive specification system supporting multiple hives with directional orientations

## Configuration

Parameters are defined in `polybee.cfg` with format `key=value`:

```
env-width=800
env-height=800  
num-bees=1
bee-max-dir-delta=0.5
num-iterations=2000
visualise=true
vis-delay-per-step=100
hive=400,400:0  # x,y:direction (0=North,1=East,2=South,3=West)
```

## Dependencies

- **C++20**: Uses std::format and other modern C++ features
- **Raylib 5.5**: For graphics and visualization (auto-downloaded via FetchContent)
- **Boost**: program_options component for command-line parsing
- **CMake 3.24+**: Build system requirement

## File Structure

- `src/`: Implementation files (.cpp)  
- `include/`: Header files (.h)
- `CMakeLists.txt`: Build configuration
- `polybee.cfg`: Runtime parameters
- `make-debug`/`run-debug`: Build and run scripts