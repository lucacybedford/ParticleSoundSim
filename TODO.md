# ParticleSoundSim — Development TODO

---

## In Progress

- [ ] Compare clarity metrics C50 and D50 against ISM baseline

---

## This week

> Implementation plan: `docs/angle_absorption_and_scattering.md`

- [ ] Part 0 — plumbing: thread RNG into `Particle::move()`; add `scattering` + `impedance` to `Material`/`Plane`
- [ ] Part B — diffuse scattering (Lambert cosine sampling, broadband `s`)
- [ ] Part A — angle-dependent absorption (locally-reacting impedance, Paris-formula calibration)
- [ ] Wire `s` and impedance into `Materials.hpp` per material
- [ ] Validation: `s=0` RIR regression guard; RT60 vs Sabine; scattering sweep

---

## Backlog – Issues

- [ ] Earliest bin is a direct arrival. For clearer transients, disregard first couple reflections

---

## Backlog — Core Physics

- [ ] Implement transmission through surfaces

---

## Backlog — Receiver & RIR

- [ ] Implement virtual microphone accumulation with direction

---

## Backlog — Validation

---

## Backlog — Visualisation & UI

- [ ] Integrate Dear ImGui
- [ ] Implement Dear ImGui sidebar (particle count, material sliders, play/pause, waveform)
- [ ] Add waveform display in ImGui panel (reconstructed signal at microphone)

---

## Backlog — Performance

- [ ] Profile and identify bottlenecks
- [ ] Spatial acceleration structure (BVH / uniform grid) for particle–surface intersection
- [ ] CPU parallelisation (multithreading, SIMD)
- [ ] GPU acceleration
- [ ] Progressive RIR re-baking as source/listener moves through the scene
- [ ] Enable movable listener

---

## Done

- [x] Set up C++ project with GLFW + GLM build system (CMake)
- [x] Convert to using GLM instead of Eigen
- [x] Implement basic room construction (axis-aligned bounding box to start)
- [x] Place point definition for planes inside object -> avoids recalculating at each loop
- [x] Design core data structures: particle, room geometry, surface, microphone
- [x] Per-particle frequency-band energy vector
- [x] Add particle emitter: including position, emission angle, amplitude
- [x] Implement air absorption: `e = e * exp(-m·dt)` (`m`: band-dependent atmospheric absorption coefficient)
- [x] Divide simulation into backend and frontend
- [x] Use ISO sound attenuation equation for calculating coefficients
- [x] Implement summation method for backend, offline simulation
- [x] Implement simple single band centre coefficient for visual simulation
- [x] Implement purely backend version
- [x] Define particle initial energy `e_0 = E_0 / N` (`E_0`: source power, `Δt`: emission duration, `N`: particles)
- [x] Implement RIR convolution with input sound
- [x] Implement real wall material absorption coefficients
- [x] Create three distinct rooms
- [x] Generate broadband RIR from histogram (energy → pressure, randomised phase per band)
- [x] Convolve RIR with input sound, produce output sound (FFT-based / partitioned convolution)
- [x] Allow for non-convex rooms by defining plane length
- [x] Enable passing input file as main argument
- [x] Implement per-surface IIR filters for frequency-dependent absorption at reflection
- [x] Implement material property model (absorption coefficient per surface, per band)
- [x] Implement check for time bin >= dt
- [x] Extend geometry from 2D half-planes to 3D enclosed rooms (triangulated surfaces)
- [x] Turn input into 44.1kHz sample rate to make sure to get correct pitch
- [x] Get visual app to work with 3D graphics
- [x] Implement rotating camera
- [x] The result is peak normalised. This is not the industry standard (LUFS + limiter: takes into account perceptual frequency weighting). Third option is to use absolute calibration where starting energy represents real loudness -> output is actual loudness too
- [x] Fix a reproducible test suite of rooms with known absorption coefficients
- [x] Compare to Allen1979 image-source method results
- [x] Visualise histogram accumulation – using python to make histogram plots from data
- [x] Compare RT60 against Sabine and Eyring closed-form predictions
- [x] Compare broadband IR shape and early-decay curve (EDC) against ISM baseline
