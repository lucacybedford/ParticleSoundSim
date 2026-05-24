# ParticleSoundSim — Development TODO

---

## Immediate (This week)

- [ ] Place point definition for planes inside object -> avoids recalculating at each loop
- [ ] Design core data structures: particle, room geometry, surface, microphone
- [ ] Implement particle emitter (sound source): position, emission angle, frequency band, amplitude
- [ ] Implement particle propagation step (velocity, time-of-flight, distance attenuation)

---

## In Progress

- [ ] Implement surface collision detection and response (reflection)
- [ ] Build OpenGL visualisation: room geometry, particles, microphone positions

---

## Backlog — Core Physics

- [ ] Extend geometry from 2D half-planes to 3D enclosed rooms (triangulated surfaces)
- [ ] Define particle initial energy `e_0 = W · Δt / N` (`W`: source power, `Δt`: emission duration, `N`: particles)
- [ ] Per-particle frequency-band energy vector (octave or third-octave bands)
- [ ] Implement angle-dependent absorption coefficient (per band)
- [ ] Implement per-surface IIR filters for frequency-dependent absorption at reflection
- [ ] Implement air absorption: `f(r) = exp(-m·r)` (`r`: distance, `m`: band-dependent atmospheric absorption coefficient)
- [ ] Implement material property model (absorption coefficient per surface, per band)
- [ ] Implement transmission through surfaces (partial pass-through, per band)

---

## Backlog — Receiver & RIR

- [ ] Implement virtual microphone accumulation (particle hit detection into time–energy histogram: time bin × frequency band × direction)
- [ ] Generate broadband RIR from histogram (energy → pressure, randomised phase per band)
- [ ] Convolve RIR with input sound, produce output sound (FFT-based / partitioned convolution)

---

## Backlog — Validation

- [ ] Implement Allen (1979) image-source method reference for rectangular rooms
- [ ] Compare RT60 against Sabine and Eyring closed-form predictions
- [ ] Compare broadband IR shape and early-decay curve (EDC) against ISM baseline
- [ ] Compare clarity metrics C50 and D50 against ISM baseline
- [ ] Fix a reproducible test suite of canonical rooms with known absorption coefficients

---

## Backlog — Visualisation & UI

- [ ] Integrate GLAD (OpenGL loader)
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
