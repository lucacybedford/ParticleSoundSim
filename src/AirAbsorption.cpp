#include "AirAbsorption.hpp"
#include <cmath>

// namespace for helper functions only used in this file
namespace {
// ANSI octave band numbers from 63Hz to 8kHz
constexpr std::array<int, AirAbsorption::NUM_BANDS> kBandNumbers{
    18, 21, 24, 27, 30, 33, 36, 39};

// get i-th sub-frequencies, logarithmically distributed over band
double sub_freq(int n, int i) {
  return std::pow(10.0, (n + (i - 4) / 3.0) / 10.0);
}
} // namespace

AirAbsorption::AirAbsorption(const Atmosphere &atm) {
  // precomputing absorption coefficients
  for (int b = 0; b < NUM_BANDS; ++b) {
    const int n = kBandNumbers[b];
    m_centre[b] = atm.absorption_m(std::pow(10.0, n / 10.0));
    for (int i = 0; i < NUM_SUB; ++i)
      m_sub[b][i] = atm.absorption_m(sub_freq(n, i));
  }
}

void AirAbsorption::decay_step(std::array<double, NUM_BANDS> &energies,
                               double dr) const {
  // apply absorption to each band using centre value
  for (int b = 0; b < NUM_BANDS; ++b)
    energies[b] *= std::exp(-m_centre[b] * dr);
}

void AirAbsorption::attenuate_total(std::array<double, NUM_BANDS> &energies,
                                    double r) const {
  for (int b = 0; b < NUM_BANDS; ++b) {
    // summing attenuation factors of sub-frequencies
    double sum = 0.0;
    for (int i = 0; i < NUM_SUB; ++i)
      sum += std::exp(-m_sub[b][i] * r);
    // applying average attenuation to band energy
    energies[b] *= sum / NUM_SUB;
  }
}
