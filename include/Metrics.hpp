#pragma once
#include "Bands.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// Metrics calculated from receiver's histogram
namespace metrics {

// sum a histogram across all octave bands into one broadband curve
inline std::vector<double>
broadband_energy(const std::vector<BandEnergies> &hist) {
  std::vector<double> e(hist.size(), 0.0);
  for (std::size_t i = 0; i < hist.size(); ++i)
    for (int b = 0; b < kNumBands; ++b)
      e[i] += hist[i][b];
  return e;
}

// Schroeder backward integrated energy decay curve
inline std::vector<double>
energy_decay_curve(const std::vector<double> &energy) {
  const std::size_t n = energy.size();
  std::vector<double> edc(n, 0.0);
  double acc = 0.0;
  for (std::size_t k = n; k-- > 0;) {
    acc += energy[k];
    edc[k] = acc;
  }
  return edc;
}

// EDC normalised to its start value and expressed in dB (edc_db[0] == 0).
// A floor keeps log10 finite once the curve has fully decayed.
inline std::vector<double> edc_db(const std::vector<double> &edc,
                                  double floor_db = -120.0) {
  std::vector<double> out(edc.size(), floor_db);
  if (edc.empty() || edc[0] <= 0.0)
    return out;
  const double e0 = edc[0];
  const double lin_floor = std::pow(10.0, floor_db / 10.0);
  for (std::size_t i = 0; i < edc.size(); ++i) {
    double r = edc[i] / e0;
    out[i] = 10.0 * std::log10(r > lin_floor ? r : lin_floor);
  }
  return out;
}

// Reverberation time (seconds) from the decay slope. A least-squares line is
// fitted to the dB decay curve between upper_db and lower_db (defaults give
// T20: -5 dB to -25 dB) and extrapolated to a 60 dB drop. Returns -1 if the
// curve never spans the requested range (e.g. too few particles / too short a
// run to decay that far).
inline double rt60(const std::vector<double> &edc_curve_db, double bin_width,
                   double upper_db = -5.0, double lower_db = -25.0) {
  // Collect (time, level) samples inside the fit window.
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  std::size_t count = 0;
  bool reached_lower = false;
  for (std::size_t i = 0; i < edc_curve_db.size(); ++i) {
    double y = edc_curve_db[i];
    if (y > upper_db)
      continue;
    if (y < lower_db) {
      reached_lower = true;
      break;
    }
    double x = i * bin_width;
    sx += x;
    sy += y;
    sxx += x * x;
    sxy += x * y;
    ++count;
  }
  if (!reached_lower || count < 2)
    return -1.0;

  double denom = count * sxx - sx * sx;
  if (denom == 0.0)
    return -1.0;
  double slope = (count * sxy - sx * sy) / denom; // dB per second (negative)
  if (slope >= 0.0)
    return -1.0;
  return -60.0 / slope;
}

// Clarity: 10 log10( early energy / late energy ), split at split_ms.
// C50 (split_ms = 50) for speech, C80 for music. Returns +inf if no energy
// arrives after the split (fully early decay).
inline double clarity(const std::vector<double> &energy, double bin_width,
                      double split_ms = 50.0) {
  const std::size_t split =
      static_cast<std::size_t>(std::round((split_ms * 1e-3) / bin_width));
  double early = 0.0, late = 0.0;
  for (std::size_t i = 0; i < energy.size(); ++i)
    (i < split ? early : late) += energy[i];
  if (late <= 0.0)
    return std::numeric_limits<double>::infinity();
  return 10.0 * std::log10(early / late);
}

} // namespace metrics
