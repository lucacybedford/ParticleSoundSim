#pragma once
#include "Bands.hpp"
#include "Particle.hpp"
#include <glm/glm.hpp>
#include <optional>
#include <random>
#include <vector>

using glm::dvec3;

struct Emitter {
  dvec3 x;
  // sound power of the source, applied to the output (W per band)
  std::optional<BandEnergies> source_power;
  float h_ang_start = 0;
  float h_ang_end = M_PI * 2;
  float v_ang_start = -M_PI / 2;
  float v_ang_end = M_PI / 2;
  Emitter(const dvec3 &x);
  Emitter(const dvec3 &x, float h_start, float h_end, float v_start,
          float v_end);
  void
  set_power_dB(const BandEnergies &L_W); // takes L_W re 1 pW per octave band
  std::vector<Particle> emit(unsigned int particle_num, double speed,
                             std::mt19937 &gen, unsigned int emitter_id);
};
