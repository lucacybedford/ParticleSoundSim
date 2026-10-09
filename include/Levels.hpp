#pragma once
#include "Bands.hpp"
#include <cmath>
#include <vector>

// absolute levels from receiver histograms
// the histogram must already be scaled by source power (W per band), so each
// bin holds W_b * H_b(k) in watts
namespace levels {

inline constexpr double p_ref = 20e-6; // Pa

// steady-state intensity for a constant source: I_b = W_b * sum_k H_b(k)/(pi
// r^2), where r is the receiver radius (W/m^2)
inline BandEnergies steady_intensity(const std::vector<BandEnergies> &hist,
                                     double radius) {
  BandEnergies I{};
  for (const BandEnergies &bin : hist)
    for (int b = 0; b < kNumBands; ++b)
      I[b] += bin[b];
  const double area = M_PI * radius * radius;
  for (double &i : I)
    i /= area;
  return I;
}

// plane-wave relation p^2 = rho c I, then dB re 20 uPa
inline double spl_dB(double intensity, double rho_c) {
  return 10.0 * std::log10(rho_c * intensity / (p_ref * p_ref));
}

inline BandEnergies band_spl(const BandEnergies &intensity, double rho_c) {
  BandEnergies L{};
  for (int b = 0; b < kNumBands; ++b)
    L[b] = spl_dB(intensity[b], rho_c);
  return L;
}

// overall level: energy sum across bands
inline double overall_spl(const BandEnergies &intensity, double rho_c) {
  double I = 0;
  for (double i : intensity)
    I += i;
  return spl_dB(I, rho_c);
}

} // namespace levels
