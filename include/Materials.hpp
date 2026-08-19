#pragma once
#include "Bands.hpp"
#include "Impedance.hpp"
#include "Material.hpp"

namespace materials {

inline double clamp_scattering(double s) {
  return s < 0.01 ? 0.01 : (s > 0.95 ? 0.95 : s);
}

inline Material make(std::string name, BandEnergies absorption,
                     double scattering) {
  return Material{std::move(name), absorption, impedance::calibrate(absorption),
                  clamp_scattering(scattering)};
}

inline const Material mStone =
    make("stone", {0.08, 0.08, 0.09, 0.12, 0.16, 0.2, 0.2, 0.2}, 0.40);
inline const Material mConcrete =
    make("concrete", {0.01, 0.02, 0.03, 0.03, 0.03, 0.04, 0.07, 0.09}, 0.15);
inline const Material mCarpet =
    make("carpet", {0.18, 0.2, 0.25, 0.3, 0.3, 0.3, 0.3, 0.4}, 0.30);
inline const Material mGlass =
    make("glass", {0.3, 0.3, 0.2, 0.1, 0.07, 0.05, 0.02, 0.02}, 0.05);
inline const Material mPlaster =
    make("plaster", {0.04, 0.04, 0.05, 0.06, 0.08, 0.04, 0.06, 0.06}, 0.20);
inline const Material mPlasterboard = make(
    "plasterboard", {0.30, 0.29, 0.10, 0.05, 0.04, 0.07, 0.09, 0.09}, 0.25);
inline const Material mSolidWood =
    make("wood", {0.19, 0.19, 0.23, 0.25, 0.30, 0.37, 0.42, 0.42}, 0.30);
inline const Material mAbsorber =
    make("absorber", {0.45, 0.65, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00}, 0.60);
} // namespace materials
