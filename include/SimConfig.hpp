#pragma once

struct SimConfig {
  // accuracy of simulation (air absorption/diffusion)
  enum class Fidelity { Offline, Realtime };

  unsigned int num_particles = 1000; // particles per emitter
  double dt = 0.02;                  // simulation time step (s)
  double max_time = 60.0;            // offline max run time (s)

  double playback_speed = 1.0;

  Fidelity fidelity = Fidelity::Offline;

  bool deterministic = false;
  unsigned int seed = 0;
};
