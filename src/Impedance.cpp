#include "Impedance.hpp"
#include <cmath>

namespace impedance {

namespace {
// to account for the peak of alpha_random from impedance reconstruction where
// max alpha possible is ~0.951
constexpr double XI_PEAK = 1.5669;
constexpr double XI_MAX = 1.0e5;
} // namespace

// from the Paris formula integration
double alpha_random(double xi) {
  const double bracket =
      xi * (xi + 2.0) / (xi + 1.0) - 2.0 * std::log(xi + 1.0);
  return 8.0 / (xi * xi) * bracket;
}

double calibrate_impedance(double alpha) {
  // clamp to the most-absorptive impedance the model allows.
  if (alpha >= alpha_random(XI_PEAK))
    return XI_PEAK;
  if (alpha <= alpha_random(XI_MAX))
    return XI_MAX;

  double lo = XI_PEAK, hi = XI_MAX;
  for (int i = 0; i < 100 && (hi - lo) > 1e-9; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (alpha_random(mid) > alpha)
      lo = mid;
    else
      hi = mid;
  }
  return 0.5 * (lo + hi);
}

BandEnergies calibrate(const BandEnergies &absorption) {
  BandEnergies xi{};
  for (std::size_t b = 0; b < absorption.size(); ++b)
    xi[b] = calibrate_impedance(absorption[b]);
  return xi;
}

} // namespace impedance
