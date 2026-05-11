# ParticleSoundSim — Development TODO

---

## Immediate

- [ ] Place point definition for planes inside object -> avoids recalculating at each loop
- [ ] Design core data structures: particle, room geometry, surface, microphone

---

## In Progress

- [ ] Implement particle emitter (sound source): position, emission angle, frequency, amplitude
- [ ] Implement particle propagation step (velocity, time-of-flight, distance attenuation)
- [ ] Implement surface collision detection and response (reflection)
- [ ] Build OpenGL visualisation: room geometry, particles, microphone positions

---

## Backlog

- [ ] Define particle initial energy `e_0 = W/N * \del t` (`W`: source power, `N`: particles, `\del t`: sound step)
- [ ] Implement angle-dependent absorption coefficient
- [ ] Implement air absorption probability: `f(r) = exp(-mr)` (`r`: distance, `m`: atmospheric absorption coefficient)
- [ ] Generate RIR
- [ ] Convolve RIR with input sound, produce output sound
- [ ] Implement virtual microphone accumulation (particle hit detection + signal reconstruction)
- [ ] Implement material property model (absorption coefficient per surface)
- [ ] Implement transmission through surfaces (partial pass-through)
- [ ] Implement Dear ImGui sidebar (particle count, material sliders, play/pause, waveform)
- [ ] Add waveform display in ImGui panel (reconstructed signal at microphone)
- [ ] Profile and optimise: identify bottlenecks, apply parallelisation
- [ ] Validate simulation output against analytical solutions (e.g. known RIR for simple rooms)
- [ ] Employ GPU acceleration instead of relying on CPU
- [ ] Enable movable listener

---

## Done

- [x] Set up C++ project with GLFW + GLAD + GLM + Dear ImGui build system (CMake)
- [x] Convert to using GLM instead of Eigen
- [x] Implement basic room construction (axis-aligned bounding box to start)
