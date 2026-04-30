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

- **Eigen3** (≥ 5.0) — installed system-wide via package manager, found via `find_package`
- **GLFW** — vendored as a static arm64 library under `include/GLFW/` (headers + `lib-arm64/libglfw3.a`); not fetched at configure time
- **OpenGL / Cocoa / IOKit / CoreVideo** — linked as macOS system frameworks; the build is Mac-only in its current form

## Architecture

The project is in early 2D proof-of-concept stage. All logic lives in `src/main.cpp`.

**Core abstractions:**

- `Plane` — an infinite half-plane defined by a surface normal `n`, an anchor point `p`, and a display length `l`. Collision detection uses the parametric ray–plane intersection `t = n·(p−x) / n·(v·dt)`.
- `Point` — a sound particle with position `x`, velocity `v`, and a sub-step collision loop (`MAX_ITERATIONS = 5`). On each `update()` call it finds the nearest plane hit within the remaining timestep, moves to the contact point, reflects `v` specularly (`v − 2(v·n)n`), and continues with the leftover `dt`. The `hit()` method is the hook for energy/absorption updates (currently a no-op).
- `diamondPlanes()` — factory that builds a square room from four diagonal planes, used as the test scene.

**Intended target design** (not yet implemented — see README):

The goal is a frequency-resolved, offline room impulse response (RIR) generator. The particle model above becomes the propagation engine; each particle will carry a per-frequency-band energy vector that is attenuated by frequency-dependent absorption coefficients and surface IIR filters (high-pass/low-pass) at each reflection. A receiver volume accumulates arrivals into a time-energy histogram (time bin × frequency band × direction) which is exported as a convolvable RIR.
