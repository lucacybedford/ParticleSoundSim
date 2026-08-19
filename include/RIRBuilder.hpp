#pragma once
#include "Bands.hpp"
#include <vector>

struct RIRBuilder {
  int sample_rate = 44100;
  double bin_width = 0.001; // seconds per histogram bin
  unsigned seed = 1234;     // RNG seed for the noise carrier

  std::vector<float> build(const std::vector<BandEnergies> &histogram) const;
};
