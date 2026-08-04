#pragma once
#include "Bands.hpp"
#include "Impedance.hpp"
#include "Material.hpp"

namespace materials {

// Scattering rises with frequency: a surface is smooth to a wavelength much
// larger than its relief and rough to one much smaller. Materials quote a
// single 1 kHz figure, and these multipliers spread it across the octave
// bands, following the shape of published measurements for architectural
// surfaces (roughly a doubling from 1 kHz to 8 kHz, a fifth of it at 63 Hz).
inline constexpr std::array<double, kNumBands> kScatterShape{
    0.20, 0.30, 0.45, 0.70, 1.00, 1.35, 1.70, 2.00};

// A scattering coefficient is the probability that a reflection is diffuse, so
// it is clamped into (0, 1). The floor keeps even glass from being a perfect
// mirror — nothing architectural is.
inline BandEnergies scatter_curve(double mid_scattering) {
  BandEnergies s{};
  for (int b = 0; b < kNumBands; ++b) {
    double v = mid_scattering * kScatterShape[b];
    s[b] = v < 0.01 ? 0.01 : (v > 0.95 ? 0.95 : v);
  }
  return s;
}

// NB the last argument is the 1 kHz scattering coefficient, not the whole
// curve: passing a bare double here used to aggregate-initialise scattering[0]
// and leave bands 1-7 at zero.
inline Material make(std::string name, BandEnergies absorption,
                     double mid_scattering) {
  return Material{std::move(name), absorption, impedance::calibrate(absorption),
                  scatter_curve(mid_scattering)};
}

// Every surface in a scene is a flat, featureless plane, so the scattering
// coefficient is the only place the relief of a real surface can enter — the
// skirting, picture rail, radiator and shelving that a plane does not have.
// These 1 kHz figures are therefore for the surface as built rather than for
// an idealised sample of the bare material; a genuinely flat sheet like
// glazing is the one that stays near zero.
inline const Material mStone =
    make("stone", {0.08, 0.08, 0.09, 0.12, 0.16, 0.2, 0.2, 0.2}, 0.40);
inline const Material mConcrete =
    make("concrete", {0.01, 0.02, 0.03, 0.03, 0.03, 0.04, 0.07, 0.09}, 0.15);
inline const Material mCarpet =
    make("carpet", {0.18, 0.2, 0.25, 0.3, 0.3, 0.3, 0.3, 0.4}, 0.30);
inline const Material mGlass =
    make("glass", {0.3, 0.3, 0.2, 0.1, 0.07, 0.05, 0.02, 0.02}, 0.05);
// Hard plaster on a solid backing — masonry walls, so almost no low-frequency
// give. For a domestic stud wall use mPlasterboard instead.
inline const Material mPlaster =
    make("plaster", {0.04, 0.04, 0.05, 0.06, 0.08, 0.04, 0.06, 0.06}, 0.20);
// Plasterboard on studs over a cavity: a panel absorber, so it takes a large
// bite out of the bottom two octaves and is nearly reflective above 500 Hz.
// This is what an ordinary house is lined with, and leaving it out is what
// leaves a domestic room with a bass tail it would not have in life.
inline const Material mPlasterboard =
    make("plasterboard", {0.30, 0.29, 0.10, 0.05, 0.04, 0.07, 0.09, 0.09},
         0.25);
inline const Material mSolidWood =
    make("wood", {0.19, 0.19, 0.23, 0.25, 0.30, 0.37, 0.42, 0.42}, 0.30);
inline const Material mAbsorber =
    make("absorber", {0.45, 0.65, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00}, 0.60);
} // namespace materials
