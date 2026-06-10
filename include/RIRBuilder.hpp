#pragma once
#include <array>
#include <vector>

struct RIRBuilder {
  int sample_rate = 44100;
  double bin_width = 0.001; // seconds per histogram bin (matches Receiver)
  unsigned seed = 1234;     // RNG seed for the noise carrier

  // ISO/ANSI octave-band centres for the 8 bands
  std::array<double, 8> band_centres{63.0,   125.0,  250.0,  500.0,
                                     1000.0, 2000.0, 4000.0, 8000.0};

  std::vector<float>
  build(const std::vector<std::array<double, 8>> &histogram) const;
};
