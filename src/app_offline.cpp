#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <cstdio>

int main() {
  SimConfig cfg;
  Atmosphere air;

  Simulation sim(make_diamond_scene(), cfg, air);

  std::printf("Speed of sound: %.2f m/s (T = %.1f C)\n", air.sound_speed(),
              air.temperature_c);

  sim.run_offline();

  std::printf("Offline run finished at t = %.3f s, %zu particles still alive\n",
              sim.time, sim.particles.size());

  for (std::size_t i = 0; i < sim.scene.receivers.size(); ++i) {
    const auto &hist = sim.scene.receivers[i].histogram;

    // sum energy across all bands and bins
    // return earliest sound detection
    double total = 0;
    int first_bin = -1;
    for (std::size_t b = 0; b < hist.size(); ++b) {
      double bin_total = 0;
      for (double e : hist[b])
        bin_total += e;
      total += bin_total;
      if (first_bin < 0 && bin_total > 0)
        first_bin = static_cast<int>(b);
    }

    double first_ms =
        first_bin < 0 ? -1 : first_bin * Receiver::bin_width * 1e3;
    std::printf("Receiver %zu: %zu time bins, first arrival = %.1f ms, total "
                "energy = %g\n",
                i, hist.size(), first_ms, total);
  }

  return 0;
}
