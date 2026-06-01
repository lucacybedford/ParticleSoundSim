#pragma once
#include <array>
#include <vector>

// Converts a receiver's energy histogram into an audio-rate pressure impulse
// response. Stochastic particle tracing yields *energy* per octave band per
// time bin -- which has no sign and no fine structure, so it cannot be
// convolved directly. For each band we shape a band-limited noise carrier by
// the sqrt of the energy envelope (energy -> pressure amplitude), then sum the
// bands. The noise reintroduces the random phase / dense reflection structure
// that the energy representation discarded.
struct RIRBuilder {
  int sample_rate = 44100;
  double bin_width = 0.001; // seconds per histogram bin (matches Receiver)
  unsigned seed = 1234;     // RNG seed for the noise carrier

  // ISO/ANSI octave-band centres for the 8 bands (63 Hz .. 8 kHz), matching
  // the bands used by AirAbsorption.
  std::array<double, 8> band_centres{63.0,   125.0,  250.0,  500.0,
                                     1000.0, 2000.0, 4000.0, 8000.0};

  // histogram[bin][band] = arriving energy. Returns a mono pressure RIR.
  std::vector<float>
  build(const std::vector<std::array<double, 8>> &histogram) const;
};
