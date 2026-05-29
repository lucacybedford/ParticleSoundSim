#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <cstdio>

// Back-end driver: no window, no OpenGL. Build the same scene, run it to
// completion as fast as the CPU allows, then report the receiver histograms.
// This is where RIR export will eventually hang off (see TODO.md).
int main() {
  SimConfig cfg;
  Simulation sim(make_diamond_scene(20), cfg);

  sim.run_offline();

  std::printf("Offline run finished at t = %.3f s, %zu particles still alive\n",
              sim.time, sim.particles.size());

  for (std::size_t i = 0; i < sim.scene.receivers.size(); ++i) {
    const auto &hist = sim.scene.receivers[i].histogram;

    // Sum energy across all bands and bins, just to show the histogram filled.
    double total = 0;
    for (const auto &bin : hist)
      for (double e : bin)
        total += e;

    std::printf("Receiver %zu: %zu time bins, total captured energy = %g\n", i,
                hist.size(), total);
  }

  return 0;
}
