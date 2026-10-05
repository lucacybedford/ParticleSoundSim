#pragma once
#include "Plane.hpp"
#include <random>
#include <vector>

struct Receiver;
struct AirAbsorption;

struct Particle {
  static constexpr int MAX_ITERATIONS = 15;
  // static constexpr double energy_threshold = 1e-6;
  static constexpr double energy_threshold = 1e-6;
  double initial_energy;
  double vel;
  dvec3 x;
  dvec3 v;
  bool alive = true;
  BandEnergies energies; // initialised to 1.0/N per band in the constructor

  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &h_angDist,
           std::uniform_real_distribution<double> &v_angDist, dvec3 &position,
           double speed, unsigned int num_particles);

  void hit(Plane &plane, double cos_theta);
  // summation is only defined for offline mode to employ accurate absorption
  // calculations
  void move(double time, double dt, std::vector<Plane> &planes,
            std::vector<Receiver> &receivers, const AirAbsorption *summation,
            std::mt19937 &rng);
  void absorb();
  double check_energy();
};
