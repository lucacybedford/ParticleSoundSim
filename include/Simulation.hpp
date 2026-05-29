#pragma once
#include "Particle.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include <vector>

// Simulation owns the dynamic state (the live particles + the clock) and the
// rule for advancing it. It is completely free of rendering: the visual app
// calls step() once per drawn frame, the offline app calls run_offline() to
// blast through to completion. Same physics, two drivers.
struct Simulation {
  Scene scene;                    // the room (walls/emitters/receivers)
  SimConfig cfg;                  // tunable parameters
  std::vector<Particle> particles; // live sound particles
  double time = 0;                // simulation clock (seconds)

  // Builds the scene's particles by emitting from every emitter.
  Simulation(Scene scene, SimConfig cfg);

  // Advance the whole simulation by dt: move every particle, deposit any that
  // reach a receiver, cull dead ones, then tick the clock. This is exactly the
  // loop body that used to be inline in main().
  void step(double dt);

  // Headless driver: keep stepping until the room is silent (all particles
  // dead) or we pass the configured cutoff time.
  void run_offline();
};
