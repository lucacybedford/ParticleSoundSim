#include "Emitter.hpp"
#include <random>

Emitter::Emitter(const dvec3 &x) : x(x) {}

Emitter::Emitter(const dvec3 &x, float h_start, float h_end, float v_start,
                 float v_end)
    : x(x), h_ang_start(h_start), h_ang_end(h_end), v_ang_start(v_start),
      v_ang_end(v_end) {}

std::vector<Particle> Emitter::emit(const int num_particles, double speed) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> h_angDist(h_ang_start, h_ang_end);
  std::uniform_real_distribution<double> v_angDist(std::sin(v_ang_start),
                                                   std::sin(v_ang_end));
  std::vector<Particle> particles;
  particles.reserve(num_particles);
  for (int i = 0; i < static_cast<int>(num_particles); i++) {
    particles.emplace_back(gen, h_angDist, v_angDist, x, speed);
  }
  // Should employ NRVO for building directly at call location
  return particles;
}
