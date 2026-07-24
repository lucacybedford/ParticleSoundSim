#pragma once
#include "Particle.hpp"
#include <glm/glm.hpp>
#include <random>
#include <vector>

using glm::dvec3;

struct Emitter {
  dvec3 x;
  float h_ang_start = 0;
  float h_ang_end = M_PI * 2;
  float v_ang_start = -M_PI / 2;
  float v_ang_end = M_PI / 2;
  Emitter(const dvec3 &x);
  Emitter(const dvec3 &x, float h_start, float h_end, float v_start,
          float v_end);
  std::vector<Particle> emit(int particle_num, double speed,
                             std::mt19937 &gen);
};
