#pragma once
#include <glm/glm.hpp>
#include <vector>

using glm::dvec2;

struct Particle;

struct Receiver {
  dvec2 x;
  float size;                                // radius
  static constexpr double bin_width = 0.001; // 1 ms bin size
  std::vector<std::array<double, 8>> histogram;
  Receiver(double x, double y, float size);
  void receive(double time, std::array<double, 8> &energies);
};
