#pragma once
#include "Atmosphere.hpp"
#include <array>

// Octave-band air-absorption coefficients (ISO 9613-1) precomputed for one
// Atmosphere. Built once; cheap to apply every step (online) or once at
// detection (offline). Both modes share these coefficients -- only the way the
// band is aggregated differs (Rindel 2024).
//
// 8 octave bands, 63 Hz .. 8 kHz (ANSI band numbers 18, 21, ... 39).
struct AirAbsorption {
  static constexpr int NUM_BANDS = 8;
  static constexpr int NUM_SUB = 9; // sub-frequencies per band (summation method)

  std::array<double, NUM_BANDS> m_centre{};                  // 1/m at band centre
  std::array<std::array<double, NUM_SUB>, NUM_BANDS> m_sub{}; // 1/m per sub-freq

  explicit AirAbsorption(const Atmosphere &atm);

  // ONLINE: decay every band by its centre-frequency coefficient over dr metres.
  // Accumulated over a path this is identical to applying it once at the end,
  // so the visual mode loses no accuracy for the centre-frequency method.
  void decay_step(std::array<double, NUM_BANDS> &energies, double dr) const;

  // OFFLINE: summation method over total path length r (metres).
  // energies[b] *= (1/N) * sum_i exp(-m_sub[b][i] * r)
  void attenuate_total(std::array<double, NUM_BANDS> &energies, double r) const;
};
