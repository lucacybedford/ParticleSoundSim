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

// pull a single octave band out of a histogram as its own energy curve
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

// One least-squares fit to a window of the dB decay curve.
struct LineFit {
  double slope = 0.0;    // dB per second, negative for a decaying curve
  double r_squared = 0.0; // goodness of fit
  std::size_t count = 0;  // samples inside the window
  bool ok = false;        // window spanned and slope usable
};

// Fit a line to the decay curve between upper_db and lower_db. `ok` is false
// when the curve never reaches lower_db, when the window holds fewer than two
// samples, or when the fitted slope is not negative.
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

// Acceptance limits for a decay fit, from ISO 3382-1 Annex B: the non-linearity
// parameter xi = 1000 (1 - r^2) on the T30 regression, and the curvature
// C = 100 (T30/T20 - 1) in per cent. Both are computed and reported. BOTH ARE
// DISABLED as accept tests, and deliberately so.
//
// A guard on xi was built and validated as a detector: tuned on one 100-run
// pool it flagged 2 of 2 outliers in a second pool at a different particle
// count with no false positives. It was then removed, because the runs it
// detects are not faulty. Recovering the per-bin arrivals from their decay
// curves shows every bin inside the fit window occupied and the energy no more
// concentrated than in an accepted run, so there is no sparse tail and no
// Schroeder staircase. The curves are non-linear because the sound field is.
//
// The standard room carries 2.7x more absorption on floor and ceiling
// (alpha 0.51) than on its walls (alpha 0.19), so horizontal paths that miss
// both are under-damped. Their predicted decay, from the 2D mean free path
// pi*A/P = 3.23 m at the wall absorption, is 97 dB/s against the diffuse
// field's 186 dB/s, and the measured late slope of the affected runs is
// 97 dB/s. In roughly 4% of runs enough energy reaches those grazing paths to
// bend the decay inside the T30 range. Rejecting them would discard real
// physics, and since the effect is one-sided it would bias every reported T30
// downward.
//
// Set either constant above zero to re-enable that test, for a room where a
// non-linear decay really would indicate a bad fit.
inline constexpr double kMaxNonlinearity = 0.0; // 0 disables the test
inline constexpr double kMaxCurvature = 0.0;    // 0 disables the test

// A decay estimate together with the diagnostics that say whether to trust it.
struct DecayFit {
  double rt60 = -1.0;         // seconds, -1 when the fit is unusable
  double t20 = -1.0;          // seconds, from the -5..-25 dB window
  double t30 = -1.0;          // seconds, from the -5..-35 dB window
  double curvature = 0.0;     // per cent
  double nonlinearity = 0.0;  // 1000 (1 - r^2)
  bool valid = false;
};

// Reverberation time from the decay slope, with the fit-quality guard applied.
// A least-squares line is fitted to the dB decay curve over the ISO 3382-1 T30
// evaluation range, -5 dB to -35 dB, and extrapolated to a 60 dB drop. The same
// fit is repeated over the T20 range so the two can be compared.
//
// The diagnostics are reported rather than acted on. In this room a non-linear
// decay curve is a property of the sound field and not a bad fit, so both
// acceptance tests are off by default and rt60 is the unfiltered T30. See
// kMaxNonlinearity for the measurement behind that decision, and set either
// limit above zero to re-enable it. A rejected fit returns -1, which every
// caller already treats as an invalid run.
inline DecayFit decay_fit(const std::vector<double> &edc_curve_db,
                          double bin_width,
                          double max_curvature = kMaxCurvature,
                          double max_nonlinearity = kMaxNonlinearity) {
  DecayFit d;
  const LineFit f30 = fit_decay(edc_curve_db, bin_width, -5.0, -35.0);
  if (!f30.ok)
    return d;
  d.t30 = -60.0 / f30.slope;
  d.nonlinearity = 1000.0 * (1.0 - f30.r_squared);

  const LineFit f20 = fit_decay(edc_curve_db, bin_width, -5.0, -25.0);
  if (!f20.ok)
    return d;
  d.t20 = -60.0 / f20.slope;
  d.curvature = 100.0 * (d.t30 / d.t20 - 1.0);

  if ((max_curvature > 0.0 && std::fabs(d.curvature) > max_curvature) ||
      (max_nonlinearity > 0.0 && d.nonlinearity > max_nonlinearity))
    return d;

  d.rt60 = d.t30;
  d.valid = true;
  return d;
}

// Convenience wrapper: the guarded T30, or -1 when the fit is rejected.
inline double rt60(const std::vector<double> &edc_curve_db, double bin_width) {
  return decay_fit(edc_curve_db, bin_width).rt60;
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
