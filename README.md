# ParticleSoundSim

A fast, physically-grounded acoustic room simulator that produces **room impulse responses (RIRs)** for arbitrary 3D room geometries. Feed any dry audio signal through the resulting RIR to hear how it would sound in that space — complete with early reflections and reverberation.

---

## What it does

ParticleSoundSim traces sound particles from a source through a user-defined 3D room, accumulating how energy arrives at a receiver over time and frequency. The output is a **time-energy histogram** — effectively a room impulse response — which is synthesised into a broadband RIR and convolved with any dry audio signal to simulate the acoustic character of the space.

The simulation accounts for:

- **Arbitrary 3D geometry** — rooms are collections of finite planar surfaces (each with a normal, anchor, length and height), so both convex and non-convex enclosures can be built
- **Frequency-resolved energy** — every particle carries an 8-band energy vector across the ISO octave bands from 63 Hz to 8 kHz
- **Frequency-dependent absorption** — each surface attenuates energy per band, using measured material coefficients (concrete, carpet, glass, plaster, wood, broadband absorber)
- **Angle-dependent absorption** — a locally-reacting surface impedance model, calibrated to each material's random-incidence absorption, so grazing and normal incidence differ
- **Diffuse scattering** — a per-surface scattering coefficient blends specular reflection with Lambert cosine-sampled diffuse reflection
- **Air absorption** — ISO 9613-1 atmospheric attenuation as a function of temperature, humidity and pressure, applied per band over the path travelled

---

## Architecture

Headers live in `include/`, sources in `src/`. Core types each have a `.hpp` + `.cpp` pair:

- **`Plane`** — a finite wall segment (normal, anchor, length, height); pre-computes tangents and corners. Carries a `Material`.
- **`Material` / `Materials`** — per-band absorption, impedance (calibrated from absorption) and scattering. `Materials.hpp` defines the stock materials.
- **`Particle`** — position, velocity and an 8-band energy vector. `move()` runs a sub-step collision loop: nearest-plane ray intersection, specular or diffuse reflection, per-band absorption, and air attenuation. Particles below an energy threshold are absorbed and erased.
- **`Emitter`** — emits a burst of particles from a point over a configurable angular range.
- **`Receiver`** — a point with a capture radius that accumulates arriving particle energy into a per-band, 1 ms-binned time-energy histogram.
- **`Scene`** — holds the planes, emitters and receivers; factory functions (`make_room`, `make_standard`, `make_L_room`, the test rooms) build scenes.
- **`Atmosphere` / `AirAbsorption`** — sound speed and ISO 9613-1 absorption coefficients.
- **`Simulation`** — owns the scene, config, RNG and particle list; `run_offline()` drives the headless run.
- **`RIRBuilder`** — synthesises a broadband RIR from a histogram: per octave band it band-passes white noise (Butterworth, via iir1) and scales it to match the band's arrival energy.
- **`Convolver`** — FFT-based convolution (pocketfft) of a dry signal with the RIR.
- **`ConvolveInput`** — `convolve_input_file()`: reads a dry WAV, resamples to the RIR rate if needed, convolves, and writes the wet result. Shared by the offline and standalone convolution tools.
- **`Wav`** — WAV read/write (dr_wav) and sample-rate conversion (libsamplerate).

**Executables:**

| Target | Source | Purpose |
|---|---|---|
| `ParticleSoundSim` | `app_visual.cpp` | Real-time 3D OpenGL visualisation of particle tracing (orbiting camera, scroll to zoom) |
| `ParticleSoundSimOffline` | `app_offline.cpp` | Headless run: simulate → histogram CSV → RIR → convolve with a dry input |
| `ParticleSoundSimConvolve` | `app_convolve.cpp` | Standalone, timed convolution of a pre-computed `rir.wav` with a dry input (no simulation) |

---

## Build

```bash
cmake -S . -B build
cmake --build build
```

`compile_commands.json` is emitted to `build/` automatically — point your LSP there.

Dependencies are fetched via `FetchContent`: **GLM**, **GLFW**, **pocketfft**, **libsamplerate**, **dr_libs**, and **iir1**. OpenGL is linked as a system framework, so the build is currently Mac-only.

---

## Run

**Visualisation:**

```bash
./build/ParticleSoundSim
```

**Offline pipeline** (writes histogram CSVs, `rir.wav`, and the convolved output WAV):

```bash
./build/ParticleSoundSimOffline [dry.wav]
```

The dry input defaults to `dry.wav`; pass a path to override. Scene, particle count and room material are set at the top of `app_offline.cpp`.

**Standalone convolution** (times just the read + convolve step, reusing a previously written `rir.wav`):

```bash
./build/ParticleSoundSimConvolve [dry.wav] [output.wav]
```

---

## Approach

The core method is **stochastic particle tracing** (analogous to Monte Carlo ray tracing in rendering):

1. A burst of particles is emitted from the source in random directions, each carrying a frequency-resolved energy vector.
2. At each surface intersection the particle energy is updated: a fraction is absorbed (frequency- and angle-dependent), the remainder is reflected either specularly or diffusely (scattering coefficient), and air absorption is applied over the path.
3. When a particle passes through the receiver volume it deposits its remaining energy into the time-energy histogram at the correct arrival time.
4. The process repeats until particles fall below an energy threshold or the run reaches its maximum time.

This trades wave-equation accuracy at low frequencies for speed and scalability, making it well suited to mid/high-frequency acoustics and large or complex geometries where full wave solvers are prohibitively expensive.

The RIR is peak-independent of particle count: amplitudes are calibrated by `1/√N` so doubling the particle budget reduces variance without changing the level.

---

## Project status

The 3D simulation pipeline is working end-to-end: emit → trace (absorption, angle-dependent impedance, diffuse scattering, air absorption) → time-energy histogram → broadband RIR → FFT convolution with a dry signal. Results have been validated against the image-source method (Allen 1979) and against Sabine/Eyring RT60 predictions.

**Planned / in progress:**

- [ ] Directional receiver model (log arrival direction per particle for spatial / Ambisonic rendering)
- [ ] Transmission through surfaces
- [ ] Spatial acceleration structure (BVH / uniform grid) for particle–surface intersection
- [ ] CPU parallelisation (multithreading, SIMD) and GPU acceleration
- [ ] Movable listener with progressive RIR re-baking
- [ ] Clarity-metric comparison (C50 / D50) against the ISM baseline

See `TODO.md` for the full development log.

---

## Motivation

Accurate room impulse responses are expensive to measure physically and slow to compute with wave-based methods (FEM/BEM). Geometric methods like image source and ray tracing are fast but often ignore frequency-dependent material behaviour. ParticleSoundSim aims to sit in the middle: physically plausible, frequency-aware, and fast enough to process large rooms or large batches of configurations offline.

Target applications include game audio pipelines, film post-production, architectural acoustics analysis, and synthetic data generation for machine learning models of room acoustics.
