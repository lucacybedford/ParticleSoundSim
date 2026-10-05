#include "Emitter.hpp"
#include <cmath>

Emitter::Emitter(const dvec3 &x) : x(x) {}

Emitter::Emitter(const dvec3 &x, float h_start, float h_end, float v_start,
                 float v_end)
    : x(x), h_ang_start(h_start), h_ang_end(h_end), v_ang_start(v_start),
      v_ang_end(v_end) {}

void Emitter::set_power_dB(const BandEnergies &L_W) {
  BandEnergies W;
  for (int b = 0; b < kNumBands; ++b) {
    W[b] = 1e-12 * std::pow(10, L_W[b] / 10);
  }
  source_power = W;
}

std::vector<Particle> Emitter::emit(unsigned int num_particles, double speed,
                                    std::mt19937 &gen,
                                    unsigned int emitter_id) {
  std::uniform_real_distribution<double> h_angDist(h_ang_start, h_ang_end);
  // sampling sin() to avoid clustering at poles -> uniform distribution over
  // solid angle
  std::uniform_real_distribution<double> v_angDist(std::sin(v_ang_start),
                                                   std::sin(v_ang_end));
  std::vector<Particle> particles;
  particles.reserve(num_particles);
  for (int i = 0; i < static_cast<int>(num_particles); i++) {
    particles.emplace_back(gen, h_angDist, v_angDist, x, speed, num_particles,
                           emitter_id);
  }
  return particles;
}
