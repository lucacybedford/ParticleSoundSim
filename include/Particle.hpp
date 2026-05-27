#pragma once
#include "Plane.hpp"
#include <array>
#include <random>
#include <vector>

struct Receiver;

struct Particle {
  static constexpr int MAX_ITERATIONS = 5;
  double vel = 100;
  dvec2 x{50, 50};
  dvec2 v{0.1, 0.1};
  bool alive = true;
  std::array<double, 8> energies{100, 100, 100, 100, 100, 100, 100, 100};

  Particle(double vx, double vy, double speed);
  Particle(std::mt19937 &gen, std::uniform_real_distribution<double> &realDist);

  void hit(Plane &plane);
  void move(double dt, std::vector<Plane> &planes);
  void check_receiver_collision(double time, std::vector<Receiver> &receivers);
  void absorb();
};
