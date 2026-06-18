#pragma once
#include "Plane.hpp"
#include <random>
#include <vector>

struct Receiver;
struct AirAbsorption;

struct Particle {
  static constexpr int MAX_ITERATIONS = 5;
  static constexpr double energy_threshold = 1e-6;
  double vel;
  dvec3 x;
  dvec3 v;
  bool alive = true;
  BandEnergies energies; // initialised to 1.0 per band in the constructor

  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &h_angDist,
           std::uniform_real_distribution<double> &v_angDist, dvec3 &position,
           double speed);

  void hit(Plane &plane);
  // summation is only defined for offline mode to employ accurate absorption
  // calculations
  void move(double time, double dt, std::vector<Plane> &planes,
            std::vector<Receiver> &receivers, const AirAbsorption *summation);
  void absorb();
  double check_energy();
};
