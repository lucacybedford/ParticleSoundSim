#include "ConvolveInput.hpp"
#include "Materials.hpp"
#include "RIRBuilder.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include "Wav.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static bool write_histogram_csv(const std::string &path,
                                const std::vector<std::array<double, 8>> &hist,
                                double bin_width) {
  std::ofstream out(path);
  if (!out)
    return false;

  out << "time_ms";
  for (int b = 0; b < 8; ++b)
    out << ",band" << b;
  out << ",total\n";

  for (std::size_t i = 0; i < hist.size(); ++i) {
    double t_ms = i * bin_width * 1e3;
    out << t_ms;
    double total = 0;
    for (double e : hist[i]) {
      out << "," << e;
      total += e;
    }
    out << "," << total << "\n";
  }
  return static_cast<bool>(out);
}

int main(int argc, char *argv[]) {
  SimConfig cfg;
  Atmosphere air;

  /*
  Configuration
  */

  bool standard = false;

  cfg.num_particles = 100000;

  float room_width = 10;
  float room_length = 30;
  float room_height = 3;
  Material room_material = materials::mConcrete;

  Scene room;
  if (standard) {
    room = make_standard();
  } else {
    room = make_room(room_width, room_length, room_height, room_material);
  }

  Simulation sim(room, cfg, air);

  // No dt <= bin_width guard: Particle::move records arrivals at their
  // sub-step time, so histogram resolution stays at bin_width whatever dt is.
  // See the dt ablation in TODO.md — accuracy is invariant up to 100 ms.

  std::printf("Speed of sound: %.2f m/s (T = %.1f C)\n", air.sound_speed(),
              air.temperature_c);

  std::printf("Particles: %i\n", cfg.num_particles);

  sim.run_offline();

  std::printf("Offline run finished at t = %.3f s, %zu particles still alive\n",
              sim.time, sim.particles.size());

  // save receiver information to csv file
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

    std::string csv_path;
    if (standard) {
      csv_path = "../output/standard/histogram_receiver_[" + std::to_string(i) +
                 "]_" + std::to_string(cfg.num_particles) + ".csv";
    } else {
      csv_path = "../output/histogram_receiver_[" + std::to_string(i) + "]_" +
                 std::to_string(cfg.num_particles) + ".csv";
    }
    if (write_histogram_csv(csv_path, hist, Receiver::bin_width))
      std::printf("Wrote %s\n", csv_path.c_str());
    else
      std::printf("Failed to write %s\n", csv_path.c_str());
  }

  std::string r_width = std::to_string(static_cast<int>(room_width));
  std::string r_length = std::to_string(static_cast<int>(room_length));
  std::string r_height = std::to_string(static_cast<int>(room_height));
  std::string r_material = room_material.name;
  std::string r_particles = std::to_string(cfg.num_particles);

  std::string input_path = "dry.wav";
  if (argc == 2) {
    input_path = argv[1];
  }

  std::string output_path;
  if (standard) {
    output_path = "../output/standard/standard-room-" + r_particles + ".wav";
  } else {
    output_path = "../output/" + r_particles + "_" + r_width + "x" + r_length +
                  "x" + r_height + "_" + r_material + "_room_" + ".wav";
  }

  // receiver converted to rir then convolved with input
  if (!sim.scene.receivers.empty()) {
    RIRBuilder builder;
    builder.bin_width = Receiver::bin_width;

    std::vector<float> rir = builder.build(sim.scene.receivers[0].histogram);
    if (rir.empty()) {
      std::printf("RIR is empty (no energy reached the receiver).\n");
      return 0;
    }

    // calibration: dividing amplitude by sqrt(N) makes RIR independent of N
    const float cal = 1.0f / std::sqrt(static_cast<float>(cfg.num_particles));
    for (float &v : rir)
      v *= cal;

    Audio rir_audio{builder.sample_rate, rir};
    if (!wav_write("rir.wav", rir_audio)) {
      std::printf("Failed to write rir.wav\n");
      return 1;
    }
    std::printf("Wrote rir.wav (%zu samples, %.3f s)\n", rir.size(),
                rir.size() / static_cast<double>(builder.sample_rate));

    // reading and convolving with input
    if (!convolve_input_file(input_path, rir, builder.sample_rate,
                             output_path)) {
      return 1;
    }
  }

  return 0;
}
