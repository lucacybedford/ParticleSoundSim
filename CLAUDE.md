# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -S . -B build
cmake --build build
```

This produces four executables in `build/`:

- `ParticleSoundSim` (`src/app_visual.cpp`) — real-time 3D OpenGL visualisation of particle tracing.
- `ParticleSoundSimOffline` (`src/app_offline.cpp`) — headless run: simulate → histogram CSV → RIR → convolve with a dry input.
- `ParticleSoundSimConvolve` (`src/app_convolve.cpp`) — standalone, timed convolution of a pre-computed `rir.wav` with a dry input (no simulation).
- `ParticleSoundSimExperiments` (`src/app_experiments.cpp`) — the measurement harness that produces everything under `output/experiments/`. Takes a mode argument.

```bash
./build/ParticleSoundSim
./build/ParticleSoundSimOffline [dry.wav]
./build/ParticleSoundSimConvolve [rir.wav] [dry.wav] [output.wav]
./build/ParticleSoundSimExperiments <variance|sweep|config|edc|scatter|convolve|decay>
```

The experiment and offline apps write to paths relative to `build/`, so run them from there.

`CMAKE_BUILD_TYPE` defaults to `Release` (`-O3 -DNDEBUG`). This matters for any reported timing: an unoptimised build runs the simulation roughly 16x slower for identical results.

`compile_commands.json` is emitted to `build/` automatically (`CMAKE_EXPORT_COMPILE_COMMANDS ON`) — point your LSP there.

There are no tests yet (`tests/` is a placeholder).

## Dependencies

All fetched via `FetchContent` except OpenGL:

- **GLM** (1.0.1) — vector/matrix math
- **GLFW** (3.4) — windowing for the visual app
- **pocketfft** (`cpp` branch) — FFT backend for convolution
- **libsamplerate** (0.2.2) — sample-rate conversion
- **dr_libs** (pinned commit) — single-header WAV read/write (`dr_wav`)
- **iir1** (1.9.5) — Butterworth band-pass filters for RIR synthesis
- **OpenGL** — linked as a system framework; the build is Mac-only in its current form

## Architecture

A frequency-resolved room impulse response (RIR) generator using stochastic particle tracing in **3D**. Headers live in `include/`, sources in `src/`. The reusable simulation/audio code is compiled into a `core` static library; the three `app_*.cpp` files are thin front-ends that link it.

Everything frequency-dependent is sized from `kNumBands` (8) in `Bands.hpp`, covering the ISO octave bands 63 Hz–8 kHz. `BandEnergies` is the `std::array<double, 8>` used for particle energy, absorption, impedance, scattering, air absorption, and receiver histograms.

**Core types (each has a `.hpp` + `.cpp` pair unless noted):**

- `Plane` — a finite wall segment (normal `n`, anchor `p`, length `l`, height `h`); pre-computes tangents and the four corners. Carries a `Material`.
- `Material` (header-only) / `Materials` (header-only) — per-band `absorption` and `impedance`, plus a single broadband `scattering` coefficient. `Materials.hpp` defines the stock materials (stone, concrete, carpet, glass, plaster, plasterboard, wood, absorber); `impedance` is calibrated from `absorption` via `Impedance`.
- `Particle` — position, velocity, and a `BandEnergies` vector (initialised to 1.0/band). `move()` runs a sub-step collision loop (`MAX_ITERATIONS = 15`): nearest-plane ray intersection, specular **or** Lambert-cosine diffuse reflection (per-surface scattering coefficient), angle- and frequency-dependent absorption, and air attenuation. Particles below `energy_threshold` are flagged `alive = false` by `absorb()` and erased via erase-remove in the sim loop. A particle that reaches a receiver deposits its energy and is then absorbed, so the receiver is a perfect absorber.
- `Emitter` — emits a burst of particles from a point over a configurable spherical angular range.
- `Receiver` — a point with a capture radius. Accumulates arriving particle energy into a per-band, 1 ms-binned time-energy histogram (`std::vector<BandEnergies>`). No arrival direction is stored yet (directional receiver is a planned feature).
- `Scene` — holds `planes`, `emitters`, `receivers`; factory functions build scenes: `make_room` (shoebox), `make_standard` (the ISM validation room), `make_L_room`, `make_cathedral`, `make_common_room`, `make_coupled_rooms`, `make_diamond_scene`.
- `Atmosphere` / `AirAbsorption` — sound speed and ISO 9613-1 atmospheric absorption. `AirAbsorption` precomputes per-band coefficients (with sub-frequencies); `attenuate_total` is the offline path, `decay_step` the real-time path.
- `Simulation` — owns the `Scene`, `SimConfig`, `Atmosphere`, `AirAbsorption`, particle list, and RNG. `run_offline()` drives the headless run; `step()` advances one frame.
- `RIRBuilder` — synthesises a broadband RIR from a histogram: per octave band it band-passes white noise (Butterworth, iir1) and scales it to match that band's total arrival energy.
- `Convolver` — FFT-based convolution (`convolve(x, h)`, pocketfft) of a dry signal with the RIR.
- `ConvolveInput` — `convolve_input_file(input_path, rir, rir_sample_rate, output_path)`: reads a dry WAV, resamples to the RIR rate if needed, convolves, and writes the wet result. Shared by `app_offline` and `app_convolve`.
- `Wav` — `Audio` struct plus WAV read/write (dr_wav), peak normalisation, and resampling (libsamplerate).

**Circular dependency:** `Particle` and `Receiver` reference each other. Both headers use forward declarations; the full `#include` happens only in the `.cpp` files.

**Offline loop (`app_offline.cpp`):** configure scene / particle count / material at the top of `main()`, then `sim.run_offline()`. Afterwards each receiver's histogram is written to CSV, the first receiver's histogram is turned into an RIR (calibrated by `1/√N` so the level is independent of particle count), written to `rir.wav`, and convolved with the dry input via `convolve_input_file`.

**Config (`SimConfig`):** `num_particles` (per emitter), `dt`, `max_time`, `playback_speed`, and a `Fidelity` flag (`Offline` uses accurate summed absorption; `Realtime` uses per-step approximations). `dt` is independent of `Receiver::bin_width`: `Particle::move` resolves the exact crossing time within a step, so arrivals are binned at 1 ms regardless of step size.

## Status

The 3D pipeline works end-to-end and has been validated against the image-source method (Allen 1979) and Sabine/Eyring RT60 predictions. See `TODO.md` for the development log and open items (directional receiver, surface transmission, spatial acceleration structures, parallelisation, movable listener).
