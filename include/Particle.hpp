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
  double vel; // particle speed in m/s == speed of sound; set at construction
  dvec2 x{50, 50};
  dvec2 v{0.1, 0.1};
  bool alive = true;
  std::array<double, 8> energies{1, 1, 1, 1, 1, 1, 1, 1};

  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &angDist,
           dvec2 &position, double speed);

  void hit(Plane &plane);
  void move(double dt, std::vector<Plane> &planes);
  // `summation` is non-null only in offline mode: when set, the particle's
  // energy is attenuated by the ISO summation method over its total path
  // (vel*time) at the moment of detection. In online mode air absorption has
  // already been applied per-step during flight, so it is left null.
  void check_receiver_collision(double time, std::vector<Receiver> &receivers,
                                const AirAbsorption *summation);
  void absorb();
  double check_energy();
};
