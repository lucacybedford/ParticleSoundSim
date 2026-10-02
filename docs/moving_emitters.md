# Design Note — Moving Emitters (baked positions + crossfade)

Status: **proposed, not started**. Design discussion from 01-10-2026, before any code.

Approach (Luca's): follow Steam Audio — bake RIRs at several emitter positions and crossfade
between them as the emitter moves, rather than simulating continuous emission at runtime.

## Why this approach

- Reuses the static RIR pipeline that is already validated (ISM, Sabine/Eyring) and the
  convolution path that already runs in real time.
- Fits the approach-noise use case: receivers (houses) are fixed and the flight path is known in
  advance, so source positions only need baking **along the path** — a line of points, not a 3D
  grid. This is Steam Audio's "static listener" baking variant.
- Supersedes the "Progressive RIR re-baking as source/listener moves" backlog item.

## Design points

### 1. Handle the direct sound separately

Each baked RIR has its direct arrival at a fixed delay. Crossfading two RIRs whose direct sound
arrives at different times gives a doubled direct sound and a comb-filter sweep, not a smooth
change. As understood (unverified against Steam Audio's docs), Steam Audio bakes only the
reflections and computes the direct path at runtime.

Plan: bake **reflections only**; compute the direct path at runtime as a per-sample variable delay
line (distance attenuation `1/r²` in energy, air absorption per band). This gives propagation
delay and Doppler for free, and the direct path dominates the level in an aircraft fly-over.

### 2. Crossfading noise-synthesised RIRs

`RIRBuilder` synthesises each band from noise. If two baked IRs are uncorrelated, a linear
crossfade loses up to 3 dB at the midpoint. Options:

- equal-power crossfade (`sqrt` gains), or
- **preferred:** interpolate the two energy histograms, then synthesise one RIR from the result.
  No dip, and the decay shape stays physically consistent.

### 3. Probe spacing

At ~70 m/s on approach the source moves 7 m every 100 ms. Spacing should be set by how fast the
reflections change with position, not a fixed grid — measure it: bake at two spacings and compare
receiver level error.

### 4. Quasi-static approximation — keep a reference

Each baked IR treats the source as stationary. Keep continuous emission (emit particles from the
current position during `step()`, receiver histogram over absolute time) as a **ground-truth
reference only**, so the baked method's error can be reported against something independent.

## Effect on ordering

Each bake is a static impulse, so moving emitters no longer depend on absolute energy units being
done first. Absolute units are still needed for physically meaningful levels; either can start
first.
