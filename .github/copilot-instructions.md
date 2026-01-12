<!-- Auto-generated guidance for AI coding agents working on polybee -->
# Copilot instructions — polybee

Purpose: Give an AI coding agent concise, actionable context so it can make small, safe, and correct code changes in this C++ project.

- **Big picture**: polybee is an agent-based C++20 simulation of bee movement with a small executable built by CMake. Core components live in `src/` and headers in `include/`.

- **Major components**:
  - `PolyBeeCore` (`src/PolyBeeCore.cpp`, `include/PolyBeeCore.h`): main simulation controller and loop.
  - `Bee` (`src/Bee.cpp`, `include/Bee.h`): agent implementation, path recording and movement rules.
  - `Environment` (`src/Environment.cpp`, `include/Environment.h`): collision and environment model.
  - `Params` (`src/Params.cpp`, `include/Params.h`): global parameter registry. New runtime parameters must be registered here.
  - `LocalVis` (`src/LocalVis.cpp`, `include/LocalVis.h`): Raylib-based visualization.

- **Why things are structured this way**: `Params` implements a static registry so the runtime config (from `polybee.cfg` or CLI) is discoverable and centralized. Visualization and simulation are separated to keep core logic testable.

- **Build & run (practical)**:
  - Typical build (fresh):

```bash
cmake -S . -B build -G Ninja
cmake --build build --target Debug
```

  - One-line debug build script: `./make-debug` (project provides this wrapper).
  - Run (debug): `./run-debug [args]` — this calls `./build/bin/debug/polybee`.
  - Release build: set `-D CMAKE_BUILD_TYPE=Release` or use `./make-release` if present.

- **Binary/output locations**: built executables go to `build/bin/debug/` or `build/bin/release/` (configured in `CMakeLists.txt`).

- **Config and runtime**:
  - Default runtime config file: `polybee.cfg` at repository root. Parameters use `key=value` format (see `CLAUDE.md` for examples).
  - Command-line parsing uses Boost.Program_options; prefer editing `polybee.cfg` for reproducible runs.

- **Project-specific conventions**:
  - Add new runtime parameters in `include/Params.h` and implement/instantiate them in `src/Params.cpp` and ensure they are added to the registry via `initRegistry()`.
  - Use the static `Params` registry (do not create ad-hoc global variables for runtime options).
  - Visualization code is kept in `LocalVis`; changes to simulation state should be made in core classes, not in visualization helpers.

- **Dependencies & integration**:
  - Uses Raylib via CMake FetchContent (see `CMakeLists.txt`). Do not assume system-wide Raylib unless you change CMake.
  - Requires Boost (program_options, serialization), OpenCV, Pagmo (via FetchContent), Eigen and TBB for some optional features.
  - Threading support is enabled for Pagmo; linkages are controlled in `CMakeLists.txt`.

- **Common edit patterns/examples**:
  - To add a new command-line option `foo-bar`: add a static param in `include/Params.h`, instantiate in `src/Params.cpp`, and include parsing/default in `initRegistry()` so it appears in `polybee.cfg` output.
  - To add a new visualization layer: modify `LocalVis::draw()` in `src/LocalVis.cpp` and prefer drawing using existing helper routines to match style.

- **Safety & testing checklist for PRs**:
  - Small behavioral changes: run a Debug build and try `./run-debug` with a small `num-iterations` and `visualise=true` to verify no runtime crashes.
  - Config changes: add example config to `config-files/` if you change default behavior.

- **Files to inspect first for any task**:
  - `CMakeLists.txt` — build, FetchContent and output paths
  - `src/PolyBeeCore.cpp` — simulation entrypoints and initialization
  - `src/Params.cpp` / `include/Params.h` — parameter registry pattern
  - `src/LocalVis.cpp` — rendering and live-debugging hooks
  - `polybee.cfg` and scripts `make-debug` / `run-debug`

If any guidance is unclear or you want more detail on a particular subsystem (e.g. how `Hive` orientation is represented, or how heatmaps are written), tell me which area and I'll expand this file with examples and line references.
