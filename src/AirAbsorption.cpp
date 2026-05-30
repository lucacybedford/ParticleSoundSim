#include "AirAbsorption.hpp"
#include <cmath>

namespace {
// ANSI octave-band numbers n for 63, 125, 250, 500, 1k, 2k, 4k, 8k Hz.
// Band centre frequency = 10^(n/10).
constexpr std::array<int, AirAbsorption::NUM_BANDS> kBandNumbers{
    18, 21, 24, 27, 30, 33, 36, 39};

// i-th sub-frequency of a band (0-based i = 0..8), distributed logarithmically:
// f_i = 10^((n + (i-4)/3)/10)   -- centre (i=4) gives exactly 10^(n/10).
// (Notes use 1-based i=1..9 with (i-5)/3; this is the 0-based equivalent.)
double sub_freq(int n, int i) {
  return std::pow(10.0, (n + (i - 4) / 3.0) / 10.0);
}
} // namespace

AirAbsorption::AirAbsorption(const Atmosphere &atm) {
  for (int b = 0; b < NUM_BANDS; ++b) {
    const int n = kBandNumbers[b];
    m_centre[b] = atm.absorption_m(std::pow(10.0, n / 10.0));
    for (int i = 0; i < NUM_SUB; ++i)
      m_sub[b][i] = atm.absorption_m(sub_freq(n, i));
  }
}

void AirAbsorption::decay_step(std::array<double, NUM_BANDS> &energies,
                               double dr) const {
  for (int b = 0; b < NUM_BANDS; ++b)
    energies[b] *= std::exp(-m_centre[b] * dr);
}

void AirAbsorption::attenuate_total(std::array<double, NUM_BANDS> &energies,
                                    double r) const {
  for (int b = 0; b < NUM_BANDS; ++b) {
    double sum = 0.0;
    for (int i = 0; i < NUM_SUB; ++i)
      sum += std::exp(-m_sub[b][i] * r);
    energies[b] *= sum / NUM_SUB; // surviving band-energy fraction g_band(r)
  }
}
