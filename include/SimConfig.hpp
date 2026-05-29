#pragma once

// Parameters that will eventually distinguish the offline (accurate) backend
// from the real-time (approximate) visual frontend.
//
// In THIS refactor it only centralises the parameters that were previously
// local variables in main(). The fidelity switches that actually make the two
// modes differ -- playback speed, air-absorption band count, diffusion model --
// are added in a later step. Keeping them here means both apps configure the
// simulation through a single object instead of scattered constants.
struct SimConfig {
  unsigned int num_particles = 1000; // particles emitted per emitter
  double dt = 0.002;                 // simulation time step (seconds)
  double max_time = 2.0;             // offline run-to-completion cutoff (s)
};
