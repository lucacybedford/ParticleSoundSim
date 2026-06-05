# ParticleSoundSim — Development TODO

---

## In Progress

---

## This week

- [ ] Visualise histogram accumulation
- [ ] Allow for non-convex rooms by defining plane length

---

## Backlog – Issues

- [ ] Turn input into 44.1kHz sample rate to make sure to get correct pitch
- [ ] Earliest bin is a direct arrival. For clearer transients, disregard first couple reflections
- [ ] Look into Linkwitz-Riley to fix the octave biquads overlap/gap normalisation

---

## Backlog — Core Physics

- [ ] Implement angle-dependent absorption coefficient (per band)
- [ ] Implement per-surface IIR filters for frequency-dependent absorption at reflection
- [ ] Extend geometry from 2D half-planes to 3D enclosed rooms (triangulated surfaces)
- [ ] Implement material property model (absorption coefficient per surface, per band)
- [ ] Implement transmission through surfaces (partial pass-through, per band)
- [ ] The result is peak normalised. This is not the industry standard (LUFS + limiter: takes into account perceptual frequency weighting). Third option is to use absolute calibration where starting energy represents real loudness -> output is actual loudness too

---

## Backlog — Receiver & RIR

- [ ] Implement virtual microphone accumulation with direction

---

## Backlog — Validation

- [ ] Implement Allen (1979) image-source method reference for rectangular rooms
- [ ] Compare RT60 against Sabine and Eyring closed-form predictions
- [ ] Compare broadband IR shape and early-decay curve (EDC) against ISM baseline
- [ ] Compare clarity metrics C50 and D50 against ISM baseline
- [ ] Fix a reproducible test suite of canonical rooms with known absorption coefficients

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
