#pragma once
#include "Impedance.hpp"
#include "Material.hpp"

namespace materials {

// Build a Material from tabulated absorption (and optional scattering), deriving
// the per-band normalised impedance by calibration. See Impedance.hpp.
inline Material make(std::string name, BandEnergies absorption,
                     BandEnergies scattering = {}) {
  return Material{std::move(name), absorption, scattering,
                  impedance::calibrate(absorption)};
}

inline const Material mConcrete =
    make("concrete", {0.01, 0.02, 0.03, 0.03, 0.03, 0.04, 0.07, 0.09});
inline const Material mCarpet =
    make("carpet", {0.18, 0.2, 0.25, 0.3, 0.3, 0.3, 0.3, 0.4});
inline const Material mGlass =
    make("glass", {0.3, 0.3, 0.2, 0.1, 0.07, 0.05, 0.02, 0.02});
inline const Material mPlaster =
    make("plaster", {0.04, 0.04, 0.05, 0.06, 0.08, 0.04, 0.06, 0.06});
inline const Material mSolidWood =
    make("wood", {0.19, 0.19, 0.23, 0.25, 0.30, 0.37, 0.42, 0.42});
inline const Material mAbsorber =
    make("absorber", {0.45, 0.65, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00});
} // namespace materials
