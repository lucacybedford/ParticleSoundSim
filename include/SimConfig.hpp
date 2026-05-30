#pragma once

// Parameters that distinguish the offline (accurate) backend from the
// real-time (approximate) visual frontend.
//
// Note what is NOT here: the speed of sound. That is physical and lives in
// Atmosphere, shared identically by both modes. SimConfig only holds choices
// about *how to run and display* the simulation.
struct SimConfig {
  // How accurately to run the physics. The branches that actually read this
  // (air-absorption band count, diffusion model) are added when we touch
  // move()/hit(); for now it just records intent and is set by each app.
  enum class Fidelity { Offline, Realtime };

  unsigned int num_particles = 1000; // particles emitted per emitter
  double dt = 0.002;                 // simulation time step (seconds)
  double max_time = 2.0;             // offline run-to-completion cutoff (s)

  // Cosmetic, frontend only. Simulated seconds advanced per frame, relative to
  // dt. 1.0 = real time; <1 = slow-motion. This does NOT change the physics or
  // the speed of sound -- it only slices time more finely so particles appear
  // to move slowly on screen. The backend ignores it (always runs at 1.0).
  double playback_speed = 1.0;

  Fidelity fidelity = Fidelity::Offline;
};
