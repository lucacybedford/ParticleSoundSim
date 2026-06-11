#pragma once
#include "Atmosphere.hpp"
#include "Bands.hpp"

struct AirAbsorption {
  static constexpr int NUM_SUB = 9; // sub-frequencies per band

  BandEnergies m_centre{};
  std::array<std::array<double, NUM_SUB>, kNumBands> m_sub{};

  explicit AirAbsorption(const Atmosphere &atm);

  // for visual method: applies at each step
  void decay_step(BandEnergies &energies, double dr) const;

  // for offline method: averages the individual sub-frequencies over the total
  // length
  void attenuate_total(BandEnergies &energies, double r) const;
};
