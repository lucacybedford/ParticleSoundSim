#pragma once
#include "Particle.hpp"
#include <glm/glm.hpp>
#include <vector>

using glm::dvec2;

struct Emitter {
  dvec2 x;
  float ang_start = 0;
  float ang_end = M_PI * 2;
  Emitter(const dvec2 &x);
  Emitter(const dvec2 &x, float start, float end);
  std::vector<Particle> emit(int particle_num);
};
