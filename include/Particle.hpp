#pragma once
#include "Plane.hpp"
#include <array>
#include <random>
#include <vector>

struct Receiver;

struct Particle {
  static constexpr int MAX_ITERATIONS = 5;
  static constexpr double energy_threshold = 1e-6;
  double vel = 100;
  dvec2 x{50, 50};
  dvec2 v{0.1, 0.1};
  bool alive = true;
  std::array<double, 8> energies{1, 1, 1, 1, 1, 1, 1, 1};

  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &angDist,
           dvec2 &position);

  void hit(Plane &plane);
  void move(double dt, std::vector<Plane> &planes);
  void check_receiver_collision(double time, std::vector<Receiver> &receivers);
  void absorb();
  double check_energy();
};
