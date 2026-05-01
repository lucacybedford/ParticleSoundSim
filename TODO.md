# ParticleSoundSim — Development TODO

---

## Immediate

---

## In Progress

---

## Backlog

- [ ] Design core data structures: particle, room geometry, surface, microphone
- [ ] Implement particle emitter (sound source): position, emission angle, frequency, amplitude
- [ ] Implement particle propagation step (velocity, time-of-flight, distance attenuation)
- [ ] Implement surface collision detection and response (reflection)
- [ ] Implement material property model (absorption coefficient per surface)
- [ ] Implement transmission through surfaces (partial pass-through)
- [ ] Implement virtual microphone accumulation (particle hit detection + signal reconstruction)
- [ ] Build OpenGL visualisation: room geometry, particles, microphone positions
- [ ] Implement Dear ImGui sidebar (particle count, material sliders, play/pause, waveform)
- [ ] Add waveform display in ImGui panel (reconstructed signal at microphone)
- [ ] Profile and optimise: identify bottlenecks, apply parallelisation
- [ ] Validate simulation output against analytical solutions (e.g. known RIR for simple rooms)
- [ ] [Stretch] Implement offline high-accuracy simulation mode for dataset generation
- [ ] [Stretch] Train neural network on generated acoustic dataset

---

## Done

- [x] Set up C++ project with GLFW + GLAD + GLM + Dear ImGui build system (CMake)
- [x] Convert to using GLM instead of Eigen
- [x] Implement basic room construction (axis-aligned bounding box to start)
