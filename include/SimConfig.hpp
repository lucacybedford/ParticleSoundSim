#pragma once

struct SimConfig {
  // accuracy of simulation (air absorption/diffusion)
  enum class Fidelity { Offline, Realtime };

  unsigned int num_particles = 100000; // particles per emitter
  double dt = 0.002;                   // simulation time step (seconds)
  double max_time = 20.0;              // offline max run time (s)

  double playback_speed = 1.0;

  Fidelity fidelity = Fidelity::Offline;
};
