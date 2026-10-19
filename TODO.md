# ParticleSoundSim — Development TODO

---

## In Progress

### Large scenes

Before moving emitters (supervision, 19-10-2026).

- [ ] Scale study: extend the free-field experiment (`ParticleSoundSimExperiments freefield`, now d = 1–8 m) to approach-scale distances; record level error, spread over seeds, receiver hits and runtime, and the N needed to stay within a chosen error
- [ ] Design large 'outdoor' room
- [ ] Performance: time the large outdoor scene against a room at the same N (spatial acceleration / parallelisation in Not Done if it is too slow)
- [ ] Receiver: radius scaling with distance from the emitter (constant r/d), with incoming energy weighted by the distance travelled inside the receiver; compare with the current receiver in the free-field test at the same N (level error and spread vs distance); then check near the ground in the outdoor scene
- [ ] Scene files in YAML or TOML instead of `.pss`: loader, saver and Blender exporter; convert `scenes/` and check each loads to the same `Scene` as its `.pss`. Do this before cone emitters add fields
- [ ] Cone emitters: a cone from each aircraft (emitter) position, aimed at the receiver; scale per-particle energy by the cone's solid angle `Ω/4π` so SPL stays correct (free-field check); find each cone's radius by testing level error against radius in a scene with a ground (in free field any cone containing the receiver gives the right level)

### Use Blender for scene generation


### Absolute energy units

- [ ] Time-varying source: convolve `W_b(t)` with `H_b` → SPL vs time and Leq (not started; needs a source time history to convolve with)
- [ ] Normalisation checks: (a) multiply initial energy by a constant `c` — every bin must scale by `c` (bit-exact if `c` is a power of 2; nothing depends on absolute energy); (b) trace at two particle counts `N` — SPL must agree within Monte Carlo noise
- [ ] Replace the `1/√N` RIR calibration in `app_offline.cpp` with the physical scaling; choose a stated dBFS ↔ Pa reference for WAV output
- [ ] Validate: free-field direct sound vs `W/(4πd²)`; reverberant level in a room vs `L_W + 10·log10(4/A)` (Sabine diffuse field)
  - Free-field smoke test (09-10-2026, `ParticleSoundSimExperiments freefield`, N = 2M, r = 0.2 m, d = 1/2/4/8 m): within +0.08/+0.04/+0.06/−0.29 dB of `W/(4πd²)` with air absorption over `d`, all inside 1.2σ of Monte Carlo noise. Without air absorption, 8 kHz is −1.2 dB at 8 m, which is the air loss. Still to do: multiple seeds, the `r/d` bias of the capture sphere (+0.04 dB at r/d = 0.2), and the room case

---

## This week

---

## Experiments to run (for Report §4 Results)

---

## Not Done

- [ ] Make single frequency band particles
- [ ] Implement virtual microphone accumulation with direction
- [ ] Implement transmission through surfaces

- [ ] Profile and identify bottlenecks
- [ ] Spatial acceleration structure (BVH / uniform grid) for particle–surface intersection
- [ ] CPU parallelisation (multithreading, SIMD)
- [ ] GPU acceleration
- [ ] Progressive RIR re-baking as the listener moves (moving source: see Moving emitters below)
- [ ] Enable movable listener
- [ ] output sound energy heatmap (SPL on the ground; later, after Large scenes)
- [ ] waveform display
- [ ] HRTF implementation

### Moving emitters

Deferred at the 19-10-2026 supervision: Large scenes first.

Design: [docs/moving_emitters.md](docs/moving_emitters.md) — baked positions + crossfade (Steam Audio style).

- [ ] Check how Steam Audio actually splits direct and baked reflections (the design note states it from memory)
- [ ] Emitter trajectory: position as a function of time (start with a straight line, constant velocity)
- [ ] Bake along the path: run the static simulation at sampled source positions on the trajectory; store each receiver's histogram per position
- [ ] Reflections only: remove the direct arrival from baked histograms (or bake with direct excluded) so it is not doubled at runtime
- [ ] Runtime direct path: per-sample variable delay line from the current source–receiver distance, with `1/r²` energy attenuation and per-band air absorption (gives propagation delay and Doppler)
- [ ] Runtime reflections: interpolate the two nearest baked histograms by position, synthesise one RIR from the result; equal-power crossfade of RIRs as the fallback
- [ ] Probe spacing study: bake at two spacings, compare receiver level error, pick spacing from the result
- [ ] Ground-truth reference: continuous emission (emit from the current position during `step()`, receiver histogram over absolute time); report the baked method's level error against it
- [ ] Validate in free field: fly-by level time history vs analytic `1/r²` with retarded time; report LAmax and SEL
- [ ] Visual app: draw the moving emitter, its trail and the baked positions

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
- [x] Part 0 — plumbing: thread RNG into `Particle::move()`; add `scattering` + `impedance` to `Material`/`Plane`
- [x] Part B — diffuse scattering (Lambert cosine sampling, broadband `s`)
- [x] Part A — angle-dependent absorption (locally-reacting impedance, Paris-formula calibration)
- [x] Wire `s` and impedance into `Materials.hpp` per material
- [x] Validation: `s=0` RIR regression guard; RT60 vs Sabine; scattering sweep
- [x] Implement angle-dependent absorption coefficient (per band)
- [x] Implement diffusion reflection
- [x] Add method for performing purely convolution (no simulation)
- [x] Time pure convolution -> can it be real-time?
- [x] Compare clarity metrics C50 and D50 against ISM baseline
- [x] **Particle-count sweep (accuracy–cost trade-off).** N over 10 log-spaced values: 1k, 2k, 5k, 10k, 20k, 50k, 100k, 200k, 500k, 1M. 10 independent RNG seeds per N. Record mean ± std across seeds of T60, EDC-vs-ISM error, C50; plus runtime. Plot metric mean±std vs N and runtime vs N (log x). Expect std ∝ 1/√N (slope −½ on log-log) = correct Monte Carlo behaviour. Headline = the "knee" where more particles stop buying accuracy while runtime keeps rising ~linearly.
- [x] **dt ablation (does swept detection decouple accuracy from step size?).** PREREQ FIRST: raise `MAX_ITERATIONS` in `Particle.hpp` (currently 5 — too low; at large dt a particle needs ~1 iteration per bounce, e.g. ~12 bounces per 100 ms step in the standard room) and relax the `dt > bin_width` guard in `app_offline.cpp`. Then fix room + N and sweep dt from 1 ms upward. Plot T60/RIR error vs dt and runtime vs dt. Expect accuracy invariant (swept + sub-step arrival timing) while runtime drops then plateaus at the bounce-dominated floor. Keep the visual app on small dt regardless. Consider culling energy-threshold particles inside the swept loop, not once per step.
- [x] **Stage timing.** Time simulation stage and convolution stage separately (convolution via the standalone tool) so real-time feasibility of each is reported independently.
- [x] Normalise particle initial energy to `1/N` per band, so each band's total emitted energy is 1.
- [x] Make `Particle::energy_threshold` relative to initial energy (it is absolute `1e-6`). Nothing else in the tracer may depend on absolute energy, or the post-hoc scaling stops being exact
- [x] Give `Emitter` a sound power per band `W_b` (W), settable as `L_W` in dB re 1e-12 W. A scale factor applied at output, not the particles' starting energy
- [x] Receiver bins → intensity, constant source: `I_b = W_b·Σ_k H_b(k)/(πr²)` → `p² = ρc·I` → SPL dB re 20 µPa per band plus overall (energy sum over bands); `include/Levels.hpp`, `Atmosphere::air_density()`, printed per receiver by `app_offline` when every emitter has a power (09-10-2026)
- [x] Convert Planes to use polygons
- [x] Write scene parser to convert scene file to Scene object (15-10-2026, `fe3e82c`)
- [x] Scene saver writes a `Scene` to `.pss`; `app_offline` takes the scene file as an argument; hard-coded scenes saved in `scenes/` (15-10-2026, `e2aa88a`, `3e763a8`, `e428aad`)
- [x] Check that each `.pss` scene reproduces its hard-coded original (in `6297e58` the `4x7x3_wood_room` outputs changed size; not yet explained)
- [x] Write python exporter for Blender
