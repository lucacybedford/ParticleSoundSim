#pragma once
#include "Atmosphere.hpp"
#include <array>

struct AirAbsorption {
  static constexpr int NUM_BANDS = 8;
  static constexpr int NUM_SUB = 9; // sub-frequencies per band

  std::array<double, NUM_BANDS> m_centre{};
  std::array<std::array<double, NUM_SUB>, NUM_BANDS> m_sub{};

  explicit AirAbsorption(const Atmosphere &atm);

  // for visual method: applies at each step
  void decay_step(std::array<double, NUM_BANDS> &energies, double dr) const;

  // for offline method: averages the individual sub-frequencies over the total
  // length
  void attenuate_total(std::array<double, NUM_BANDS> &energies, double r) const;
};
