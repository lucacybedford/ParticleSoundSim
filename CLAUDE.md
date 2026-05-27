# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -S . -B build
cmake --build build
./build/ParticleSoundSim
```

`compile_commands.json` is emitted to `build/` automatically (`CMAKE_EXPORT_COMPILE_COMMANDS ON`) — point your LSP there.

There are no tests yet (`tests/` is a placeholder).

## Dependencies

- **GLM** (1.0.1) — fetched via `FetchContent`
- **GLFW** (3.4) — fetched via `FetchContent`
- **OpenGL** — linked as a system framework; the build is Mac-only in its current form

## Architecture

The project is a 2D proof-of-concept for a frequency-resolved room impulse response (RIR) generator using stochastic particle tracing. Headers live in `include/`, sources in `src/`.

**Core types (each has a `.hpp` + `.cpp` pair):**

- `Plane` — a finite wall segment defined by surface normal `n`, anchor point `p`, and display length `l`. Pre-computes tangent and endpoints. Carries per-surface absorption coefficients for 8 ISO octave bands.
- `Particle` — a sound particle with position, velocity, and a per-frequency-band energy vector (`std::array<double, 8>`). `move()` runs a sub-step collision loop (`MAX_ITERATIONS = 5`): finds nearest plane hit via parametric ray–plane intersection, reflects specularly, and attenuates energy by the plane's absorption coefficients. Particles are flagged `alive = false` by `absorb()` and erased from the vector in the main loop via erase-remove.
- `Receiver` — a point with a capture radius. Accumulates arriving particle energies into a pre-binned time-energy histogram (`std::vector<std::array<double, 8>>`, 1ms bins). When a particle enters the receiver volume, its energy is deposited into the correct time bin and the particle is absorbed.

**Circular dependency:** `Particle` and `Receiver` reference each other. Both headers use forward declarations; the full `#include` happens only in the `.cpp` files.

**Simulation loop (`main.cpp`):** `dt` and simulation `time` are owned by `main()` and passed into particle methods. Each frame: move particles, check receiver collisions, erase dead particles, render. `diamondPlanes()` builds the test scene (a rotated square room from four diagonal planes).

**Intended target design** (not yet implemented — see README and TODO.md): 3D geometry, distance/air attenuation, diffuse scattering, surface IIR filters, directional receiver, broadband RIR export, and convolution with dry audio.
