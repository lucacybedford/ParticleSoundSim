#pragma once
#include "Plane.hpp"
#include <array>
#include <random>
#include <vector>

struct Receiver;
struct AirAbsorption;

struct Particle {
  static constexpr int MAX_ITERATIONS = 5;
  static constexpr double energy_threshold = 1e-6;
  double vel;
  dvec2 x;
  dvec2 v;
  bool alive = true;
  std::array<double, 8> energies{1, 1, 1, 1, 1, 1, 1, 1};

  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &angDist,
           dvec2 &position, double speed);

  void hit(Plane &plane);
  void move(double dt, std::vector<Plane> &planes);
  // summation is only defined for offline mode to employ accurate absorption
  // calculations
  void check_receiver_collision(double time, std::vector<Receiver> &receivers,
                                const AirAbsorption *summation);
  void absorb();
  double check_energy();
};
