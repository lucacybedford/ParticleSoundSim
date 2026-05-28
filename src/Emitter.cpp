#include "Emitter.hpp"
#include <random>

Emitter::Emitter(const dvec2 &x) : x(x) {}

Emitter::Emitter(const dvec2 &x, float start, float end)
    : x(x), ang_start(start), ang_end(end) {}

std::vector<Particle> Emitter::emit(const int num_particles) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> angDist(ang_start, ang_end);
  std::vector<Particle> particles;
  particles.reserve(num_particles);
  for (int i = 0; i < static_cast<int>(num_particles); i++) {
    particles.emplace_back(gen, angDist, x);
  }
  // Should employ NRVO for building directly at call location
  return particles;
}
