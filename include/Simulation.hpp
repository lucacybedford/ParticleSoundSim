#pragma once
#include "AirAbsorption.hpp"
#include "Atmosphere.hpp"
#include "Particle.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include <random>
#include <vector>

// Simulation defines state and rules for updating
struct Simulation {
  Scene scene;
  SimConfig cfg; // tunable parameters
  Atmosphere atmosphere;
  AirAbsorption air; // precomputed absorption coefficients
  std::vector<Particle> particles;
  std::mt19937 rng;
  double time = 0;

  Simulation(Scene scene, SimConfig cfg, Atmosphere atmosphere);

  void step(double dt);

  void run_offline();
};
