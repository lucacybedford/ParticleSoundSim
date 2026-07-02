# Implementation Note — Angle-Dependent Absorption & Diffuse Scattering

Status: planned (not yet implemented)
Scope: `src/Particle.cpp`, `include/Material.hpp`, `include/Plane.hpp`, `src/Plane.cpp`,
`include/Materials.hpp`, `include/Simulation.hpp`, `src/Simulation.cpp`

Both features modify the same bounce site:

- `Particle::hit()` — applies absorption (`Particle.cpp:46-48`)
- the reflection block in `Particle::move()` — specular mirror at `Particle.cpp:117-127`

---

## Where things stand now

- **Reflection is purely specular**: `v_new = v - 2*dot(v, n)*n` (`Particle.cpp:124`).
- **Absorption is angle-independent**: `energies[i] *= (1 - plane.absorption[i])` (`Particle.cpp:48`), applied via `hit()`.
- The coefficients in `Materials.hpp` are **random-incidence (Sabine) coefficients** — this matters for Part A.
- `move()` has **no RNG** — diffuse scattering needs one (Part 0).
- `Plane` already stores `h_tangent` and `v_tangent` (`Plane.cpp:11-13`) — the orthonormal basis needed for Lambert sampling.

---

## Part 0 — Shared plumbing (do first)

**0.1 Thread an RNG into the bounce.**
`move()` is called per-particle in `Simulation::step` (`Simulation.cpp:21`).

- Add a `std::mt19937` member to `Simulation`.
- Add a `std::mt19937& rng` parameter to `Particle::move()` and pass it through.
- Note for later: the Performance TODO lists multithreading — when that lands, use per-thread
  RNGs. Keep the RNG passed-in (not global) so that change stays local.

**0.2 Extend `Material` / `Plane`.**
In `Material.hpp` add:

- `BandEnergies scattering;` — scattering coefficient *s* per band (Part B)
- `BandEnergies impedance;` — real normalised surface impedance ξ per band (Part A), precomputed

`Plane` currently copies only `absorption` (`Plane.cpp:8`). Copy the two new arrays too.

Checkpoint: compiles clean, no behaviour change yet.

---

## Part A — Angle-Dependent Absorption

Subtlety: the `Materials.hpp` values are random-incidence coefficients. You cannot just scale
them by angle — you need a surface model that *produces* α(θ), then make it consistent with the
tabulated value.

**A.1 Model — locally-reacting surface, real normalised impedance ξ.**

```
R(θ) = (ξ·cosθ − 1) / (ξ·cosθ + 1)      # pressure reflection coefficient
α(θ) = 1 − |R(θ)|²                       # absorption at incidence angle θ
```

**A.2 Calibrate ξ to the tabulated α once, offline.**
Invert Paris' formula per material per band so the angle-averaged absorption matches the Sabine
value already in the table:

```
α_random = ∫₀^{π/2} α(θ) · 2 sinθ cosθ dθ
```

This is a 1-D root-find for ξ given α_random. Do it once at material construction
(e.g. a `calibrate_impedance()` helper), store result in `Material::impedance`. Keeps RT60
consistent with current results while adding angle dependence on top.

**A.3 Apply at the bounce.**
In `move()`, incidence cosine is `cosθ = |dot(normalize(v), plane.n)|`.

- Change signature to `hit(Plane&, double cos_theta)`.
- Replace the loop at `Particle.cpp:46-48` with the energy reflection factor `|R(θ)|²` per band
  instead of `(1 − α)`.

Edge cases: clamp cosθ away from 0 (grazing → R→−1, α→0; correct but watch numerics);
ξ→∞ for near-perfect reflectors.

---

## Part B — Diffuse Scattering

Standard room-acoustics approach (Vorländer): scattering coefficient *s* ∈ [0,1] per band sets the
fraction of reflected energy going diffuse vs specular.

**B.1 Per-particle stochastic decision.**
At the bounce, draw `u ~ U(0,1)`. A particle can only go one direction, so use a single broadband
*s* (e.g. the mid-band value) for the directional decision.

- `u > s` → keep current specular reflection (`Particle.cpp:124`).
- `u ≤ s` → diffuse: resample direction by Lambert's cosine law about the normal.

(The fully rigorous per-band split needs either per-band particle splitting or a
Vorländer–Mommertz energy-weighted scheme — treat as a stretch. Document the broadband
simplification as a deliberate real-time tradeoff.)

**B.2 Lambert (cosine-weighted hemisphere) sampling, using the plane's tangent basis:**

```
r1, r2 ~ U(0,1)
cosθ = sqrt(r1);  sinθ = sqrt(1 − r1);  φ = 2π·r2
dir = sinθ·cosφ · h_tangent + sinθ·sinφ · v_tangent + cosθ · n
v   = normalize(dir) * vel
```

Reproduces the cosθ radiation that makes Lambert diffusion match measured reverberant fields.
Cite Kuttruff (Lambert's law) — already the theory backbone in CLAUDE.md.

**B.3 Ordering with Part A.**
Apply absorption (energy loss) **first**, using the *incident* angle, then pick the outgoing
direction. Do not apply angle-dependent absorption to the diffuse outgoing angle.

---

## Suggested work order

1. Part 0 plumbing (RNG + Material/Plane fields) — no behaviour change, compile-clean checkpoint.
2. Part B diffuse scattering with a hardcoded `s` — easiest to see in `app_visual`; RT60 should
   barely move, sound field should "fill in".
3. Part A angle-dependent absorption with the impedance calibration.
4. Wire `s` and impedance into `Materials.hpp` per material.

---

## Validation (Phase 4 hook)

- **Regression guard:** with `s = 0` and angle-independent α restored, the RIR must match current
  output. Gate this before trusting either feature.
- **Angle-dependent α:** RT60 per band should stay close to Sabine `0.161·V/A` after calibration —
  that is the point of the Paris inversion.
- **Scattering:** sweep `s` from 0→1; late decay should become smoother / more exponential (mixing).
  Keep `s = 0` for any direct ISM comparison — the Allen 1979 image-source baseline is
  specular-only.
