#pragma once
#include "Bands.hpp"
#include <glm/glm.hpp>
#include <vector>

using glm::dvec3;

struct Particle;

struct Receiver {
  dvec3 x;
  float size;                                // radius
  static constexpr double bin_width = 0.001; // 1 ms bin size
  std::vector<BandEnergies> histogram;
  Receiver(dvec3 x, float size);
  void receive(double time, BandEnergies &energies);
};
