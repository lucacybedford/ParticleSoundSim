#pragma once
#include "Bands.hpp"
#include <glm/glm.hpp>
#include <vector>

using glm::dvec3;

struct Particle;

struct Receiver {
  dvec3 x;
  // float, not double, only to keep results bit-reproducible against the
  // published result set: 0.1 is not exactly representable in float, and
  // widening it perturbs trajectories enough to change a decay tail.
  float size;                                // radius
  static constexpr double bin_width = 0.001; // 1 ms bin size
  std::vector<BandEnergies> histogram;
  Receiver(dvec3 x, float size);
  void receive(double time, BandEnergies &energies);
};
