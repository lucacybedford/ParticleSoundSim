# ParticleSoundSim

A fast, physically-grounded acoustic room simulator that produces **room impulse responses (RIRs)** for arbitrary room geometries. Feed any dry audio signal through the resulting RIR to hear how it would sound in that space — complete with echoes, reverberation, and positional arrival directions for surround-sound rendering.

---

## What it does

ParticleSoundSim traces sound particles from a source through a user-defined room geometry, accumulating how energy arrives at a receiver over time. The output is a **time-energy response** — effectively a room impulse response — which you can convolve with any dry audio signal to simulate the acoustic character of the space.

The simulation accounts for:

- **Arbitrary room geometry** — define rooms as collections of planar surfaces in any configuration
- **Frequency-dependent absorption** — each surface attenuates energy differently across the frequency spectrum (e.g. soft furnishings absorb high frequencies more than low)
- **Surface scattering coefficients** — control the balance between specular (mirror-like) and diffuse reflection per surface
- **High-pass / low-pass surface filtering** — surfaces can act as frequency filters, sculpting the tonal character of reflected sound
- **Energy-distance decay** — amplitude falls off with distance and cumulative absorption, matching real inverse-square-law behaviour
- **Directional arrival** — each particle arrival is logged with its incoming direction, enabling positional audio rendering (e.g. Ambisonics or multi-channel speaker arrays)

---

## Output

The simulator produces a multichannel, frequency-resolved **time-energy histogram**: for each time bin and frequency band, it records how much energy arrives at the receiver and from which direction. This can be used to:

- **Convolve with a dry audio signal** to add realistic room reverberation and echoes
- **Drive a spatial audio renderer** to place early reflections in 3D space (surround sound / binaural)
- **Analyse room acoustics** — extract metrics like RT60, clarity (C80), or early decay time

---

## Approach

The core method is **stochastic particle tracing** (analogous to Monte Carlo ray tracing in rendering):

1. A burst of particles is emitted from the source in random directions, each carrying a frequency-resolved energy vector.
2. At each surface intersection the particle energy is updated: a fraction is absorbed (frequency-dependent), the remainder is reflected either specularly or diffusely (scattering coefficient), and a surface filter response is applied.
3. When a particle passes through the receiver volume it deposits its remaining energy into the time-energy histogram at the correct arrival time and direction.
4. The process repeats until particles fall below an energy threshold or exceed a maximum path length.

This approach trades wave-equation accuracy at low frequencies for speed and scalability, making it well suited to mid/high-frequency acoustics and large or complex geometries where full wave solvers are prohibitively expensive.

---

## Project status

Early development. The current codebase contains a 2D proof-of-concept particle simulator with specular reflection off planar boundaries. The full 3D simulation pipeline described above is the target design.

**Planned milestones:**

- [ ] 3D geometry representation and BVH-accelerated intersection
- [ ] Frequency-band energy model per particle
- [ ] Frequency-dependent absorption and scattering per surface
- [ ] Surface IIR filter application (high-pass / low-pass effects)
- [ ] Time-energy histogram accumulation at receiver
- [ ] RIR export (WAV / numpy array)
- [ ] Convolution utility to apply RIR to a dry signal
- [ ] Directional receiver model for spatial audio output
- [ ] Parallelisation (multi-threaded particle batches)

---

## Motivation

Accurate room impulse responses are expensive to measure physically and slow to compute with wave-based methods (FEM/BEM). Geometric methods like image source and ray tracing are fast but often ignore frequency-dependent material behaviour. ParticleSoundSim aims to sit in the middle: physically plausible, frequency-aware, and fast enough to process large rooms or large batches of configurations offline.

Target applications include game audio pipelines, film post-production, architectural acoustics analysis, and synthetic data generation for machine learning models of room acoustics.
