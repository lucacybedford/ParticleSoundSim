#pragma once
#include "Bands.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// metrics calculated from receiver's histogram
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

inline std::vector<double> band_energy(const std::vector<BandEnergies> &hist,
                                       int band) {
  std::vector<double> e(hist.size(), 0.0);
  for (std::size_t i = 0; i < hist.size(); ++i)
    e[i] = hist[i][band];
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

// EDC normalised to its start value
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

// one least-squares fit to a window of the dB decay curve.
struct LineFit {
  double slope = 0.0; // dB per second
  double r_squared = 0.0;
  std::size_t count = 0; // samples inside the window
  bool ok = false;
};

// fit a line to the decay curve between upper_db and lower_db
inline LineFit fit_decay(const std::vector<double> &edc_curve_db,
                         double bin_width, double upper_db, double lower_db) {
  LineFit f;
  double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
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
    syy += y * y;
    sxy += x * y;
    ++f.count;
  }
  if (!reached_lower || f.count < 2)
    return f;

  const double n = static_cast<double>(f.count);
  const double denom = n * sxx - sx * sx;
  if (denom == 0.0)
    return f;
  f.slope = (n * sxy - sx * sy) / denom;
  if (f.slope >= 0.0)
    return f;

  const double var_y = n * syy - sy * sy;
  const double cov = n * sxy - sx * sy;
  f.r_squared = (var_y > 0.0) ? (cov * cov) / (denom * var_y) : 0.0;
  f.ok = true;
  return f;
}

struct DecayFit {
  double rt60 = -1.0; // seconds, -1 when the fit is unusable
  double t20 = -1.0;  // seconds, from the -5..-25 dB window
  double t30 = -1.0;  // seconds, from the -5..-35 dB window
  bool valid = false;
};

inline DecayFit decay_fit(const std::vector<double> &edc_curve_db,
                          double bin_width) {
  DecayFit d;
  const LineFit f30 = fit_decay(edc_curve_db, bin_width, -5.0, -35.0);
  if (!f30.ok)
    return d;
  d.t30 = -60.0 / f30.slope;

  const LineFit f20 = fit_decay(edc_curve_db, bin_width, -5.0, -25.0);
  if (!f20.ok)
    return d;
  d.t20 = -60.0 / f20.slope;

  d.rt60 = d.t30;
  d.valid = true;
  return d;
}

inline double rt60(const std::vector<double> &edc_curve_db, double bin_width) {
  return decay_fit(edc_curve_db, bin_width).rt60;
}

// clarity: 10 log10( early energy / late energy )
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
