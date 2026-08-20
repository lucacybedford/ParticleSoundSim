# ParticleSoundSim

ParticleSoundSim is a particle-based simulation of sound propagation in enclosed spaces. A source emits discrete energy-carrying particles into a room, which are reflected and absorbed by its surfaces before being accumulated at a virtual microphone to reconstruct a room impulse response, the room's response to an ideal impulse encoded as an energy-time histogram. Convolving that response with a dry recording produces the signal as it would have been heard in that space.

The method is geometric rather than wave-based, so sound is treated as particles travelling in straight lines and interacting with surfaces by energy-based rules. This discards diffraction and room resonances, but costs little enough to stay practical in rooms where solving the wave equation directly would not be. Accuracy is traded against time through the particle count and the time step, both set per run, which allows a room to be explored quickly and then simulated properly once its configuration is settled.

## What the simulation accounts for

Energy is carried per frequency band rather than broadband, across the eight ISO octave bands from 63 Hz to 8 kHz. Every mechanism below acts on each band independently, so the response varies with frequency in the way a real room does rather than decaying at a single rate.

Surfaces absorb according to measured material coefficients, and because absorption in a real room depends on the angle at which sound arrives, each material's random-incidence coefficient is converted into a surface impedance so that grazing and normal incidence differ. Reflection is not purely specular either, since a per-surface scattering coefficient decides whether a particle leaves at the mirror angle or is scattered diffusely about the surface normal, which is what prevents the artificial echoing a purely specular room produces. Air attenuates sound over the distance travelled, and this is applied per band following ISO 9613-1 as a function of temperature, humidity, and pressure, an effect that is negligible at low frequencies and dominant at high ones.

Rooms are assembled from finite planar surfaces rather than box dimensions, so enclosures need not be convex, and shapes such as an L-shaped room or two spaces coupled through an opening can be built and measured.

## What comes out

The simulation produces a time-energy histogram at each receiver, binned at 1 ms per frequency band. Arrivals are recorded at their true time within a step rather than at the end of it, so the resolution of the output is set by the bin width and not by the time step.

That histogram is then synthesised into an audible impulse response by band-passing white noise into each octave band and scaling it to match the energy that arrived in that band, and the bands are summed into one broadband response. Levels are calibrated so that the result does not depend on how many particles were used, meaning a larger particle budget reduces statistical noise without changing the loudness. The response is written as a WAV file and convolved with a dry input to produce the wet result.

Beyond listening, the histogram supports the standard measures used to describe a room. Reverberation time is estimated from the energy decay curve over the ISO 3382-1 window, and clarity is taken as the ratio of early to late arriving energy. Both have been validated against the image-source method and against closed-form Eyring-Norris predictions.

## Scope

The limitations of geometric acoustics are inherited, so neither edge diffraction nor room modes are modelled. Modal behaviour dominates below the Schroeder frequency, which falls around 130 to 144 Hz in the rooms used here, and results below that point should be read as an energy estimate rather than a description of the field. The receiver is a point with a capture radius and stores no arrival direction, so the output is monophonic.

## Building

```bash
cmake -S . -B build
cmake --build build
```

Dependencies are fetched automatically during configuration. OpenGL is linked as a system framework, so the build is currently macOS only. The build defaults to `Release`, which matters for any reported timing, as an unoptimised build runs roughly 16 times slower for identical results.

## Running

Four programs are produced in `build/`.

`ParticleSoundSim` opens a real-time 3D view of particles tracing through the room, which is useful for confirming that a scene behaves as intended before measuring it.

```bash
./build/ParticleSoundSim
```

`ParticleSoundSimOffline` runs the full pipeline without a window, writing the histogram, the impulse response, and the convolved output. The room, particle count, and material are chosen at the top of its source file.

```bash
./build/ParticleSoundSimOffline [dry.wav]
```

`ParticleSoundSimConvolve` applies a previously written impulse response to a dry signal and times the operation, with no simulation involved. Note that the response is given first.

```bash
./build/ParticleSoundSimConvolve [rir.wav] [dry.wav] [output.wav]
```

`ParticleSoundSimExperiments` is the experiment harness behind the reported results, covering run-to-run variance, particle-count and time-step sweeps, decay curves, scattering comparisons, and convolution timing.

```bash
./build/ParticleSoundSimExperiments <variance|sweep|config|edc|scatter|convolve|decay>
```

The offline and experiment programs write to paths relative to `build/`, so they should be run from there. The Python plotting scripts in the repository root read those results and produce the figures, and they expect to be run from the root rather than from `build/`.
