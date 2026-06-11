#include "RIRBuilder.hpp"
#include <Iir.h>
#include <algorithm>
#include <cmath>
#include <random>

std::vector<float>
RIRBuilder::build(const std::vector<std::array<double, 8>> &hist) const {
  const std::size_t n_bins = hist.size();
  if (n_bins == 0)
    return {};

  const double samples_per_bin = bin_width * sample_rate;
  const std::size_t L = static_cast<std::size_t>(
      std::ceil(n_bins * samples_per_bin)); // total number of samples in RIR
  const double nyquist = 0.5 * sample_rate;

  std::vector<double> rir(L, 0.0); // final output accumulator

  std::mt19937 rng(seed);
  std::normal_distribution<double> gauss(0.0, 1.0);

  std::vector<double> noise(L);
  std::vector<double> band(L);

  // loop over bands of histogram
  for (int b = 0; b < 8; ++b) {
    double target_energy =
        0.0; // accumulation of all energy in band as reference
    for (std::size_t k = 0; k < n_bins; ++k)
      target_energy += hist[k][b];
    if (target_energy <= 0.0)
      continue; // nothing arrived in this band

    // generate random noise sample for each sample in RIR
    for (std::size_t n = 0; n < L; ++n)
      noise[n] = gauss(rng);

    // bandpass the noise to this octave band: edges at fc/sqrt(2)..fc*sqrt(2),
    // upper edge clamped below Nyquist for low sample rates
    const double fc = band_centres[b];
    const double lo = fc / std::sqrt(2.0);
    const double hi = std::min(fc * std::sqrt(2.0), 0.95 * nyquist);
    Iir::Butterworth::BandPass<4> bp;
    bp.setup(sample_rate, 0.5 * (lo + hi), hi - lo);
    for (std::size_t n = 0; n < L; ++n)
      noise[n] = bp.filter(noise[n]);

    // 3. Shape by the pressure-amplitude envelope = sqrt(energy), linearly
    //    interpolating the coarse 1 ms histogram up to audio rate.
    double synth_energy = 0.0;
    for (std::size_t n = 0; n < L; ++n) {              // for each sample
      double t_bin = n / samples_per_bin;              // get bin currently in
      std::size_t k = static_cast<std::size_t>(t_bin); // get bin below
      double frac =
          t_bin - static_cast<double>(k); // get distance between bin k and k+1
      double e0 = hist[k][b];
      double e1 = (k + 1 < n_bins) ? hist[k + 1][b] : 0.0;
      double e = e0 + frac * (e1 - e0); // linearly interpolate energy
      double v = noise[n] * std::sqrt(std::max(0.0, e));
      band[n] = v;
      synth_energy += v * v;
    }

    // normalise each band's total energy
    double g = std::sqrt(target_energy / synth_energy);
    for (std::size_t n = 0; n < L; ++n)
      rir[n] += band[n] * g;
  }

  std::vector<float> out(L);
  for (std::size_t n = 0; n < L; ++n)
    out[n] = static_cast<float>(rir[n]);
  return out;
}
