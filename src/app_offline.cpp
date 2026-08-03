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

  enum class Room { Box, Cathedral, LivingRoom, CoupledRooms, Standard };
  const Room which = Room::CoupledRooms;
  const bool standard = which == Room::Standard;

  cfg.num_particles = 200000;
  cfg.dt = 0.020;

  float room_width = 30;
  float room_length = 80;
  float room_height = 25;
  // Build custom materials through materials::make, never by assigning to
  // .absorption on a copy: Particle::hit reflects off material.impedance, and
  // only make() runs impedance::calibrate() to derive it from the absorption
  // coefficients. Editing .absorption alone changes nothing the sim reads.
  Material room_material = materials::mStone;
  // Geometry tag for the output filenames, so cathedral runs do not overwrite
  // the shoebox ones.
  std::string geometry;
  Scene room;
  switch (which) {
  case Room::Standard:
    room = make_standard();
    geometry = "standard";
    break;
  case Room::Cathedral:
    room = make_cathedral(room_material);
    geometry = "cathedral_" + room_material.name;
    break;
  case Room::LivingRoom:
    // Picks its own per-surface materials, so room_material is unused here.
    room = make_common_room();
    geometry = "living_room";
    break;
  case Room::CoupledRooms:
    // Also self-materialled. Two receivers: [0] in the small room (the RIR
    // this app renders, and where the two-slope decay shows), [1] in the hall.
    room = make_coupled_rooms();
    geometry = "coupled_rooms";
    break;
  case Room::Box:
    room = make_room(room_width, room_length, room_height, room_material);
    geometry = std::to_string(static_cast<int>(room_width)) + "x" +
               std::to_string(static_cast<int>(room_length)) + "x" +
               std::to_string(static_cast<int>(room_height)) + "_" +
               room_material.name + "_room";
    break;
  }

  Simulation sim(room, cfg, air);

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
    } else if (which != Room::Box) {
      csv_path = "../output/histogram_receiver_[" + std::to_string(i) + "]_" +
                 geometry + "_" + std::to_string(cfg.num_particles) + ".csv";
    } else {
      csv_path = "../output/histogram_receiver_[" + std::to_string(i) + "]_" +
                 std::to_string(cfg.num_particles) + ".csv";
    }
    if (write_histogram_csv(csv_path, hist, Receiver::bin_width))
      std::printf("Wrote %s\n", csv_path.c_str());
    else
      std::printf("Failed to write %s\n", csv_path.c_str());
  }

  std::string r_particles = std::to_string(cfg.num_particles);

  std::string input_path = "dry.wav";
  if (argc == 2) {
    input_path = argv[1];
  }

  // Shared stem so the wet render and the RIR that produced it sit next to each
  // other in output/ under matching names. Without this the RIR is written to
  // the CWD as a fixed "rir.wav" and every room in a demo run clobbers the
  // last.
  std::string stem;
  if (standard) {
    stem = "standard/standard-room-" + r_particles;
  } else {
    stem = r_particles + "_" + geometry;
  }

  std::string output_path = "../output/" + stem + ".wav";
  std::string rir_path = "../output/" + stem + "_rir.wav";

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

    // The pipeline has no absolute SPL reference (no source power, no 1/r^2
    // calibration), so the overall scale is arbitrary and only the ratios
    // between rooms carry meaning. kDemoGain lifts every render into audible
    // range while leaving those ratios untouched.
    //
    // It MUST stay identical across every room in a comparison: retuning it
    // per room silently destroys the loudness differences the demo exists to
    // show. Change it once, then re-render the whole set.
    constexpr float kDemoGain = 4.0f; // +12 dB

    for (float &v : rir)
      v *= cal * kDemoGain;

    Audio rir_audio{builder.sample_rate, rir};
    if (!wav_write(rir_path, rir_audio)) {
      std::printf("Failed to write %s (directory must exist)\n",
                  rir_path.c_str());
      return 1;
    }
    std::printf("Wrote %s (%zu samples, %.3f s)\n", rir_path.c_str(),
                rir.size(),
                rir.size() / static_cast<double>(builder.sample_rate));

    // Also keep the fixed-name copy in the CWD: app_convolve reads "rir.wav"
    // from there by default, so this preserves that workflow.
    if (!wav_write("rir.wav", rir_audio))
      std::printf("Failed to write rir.wav\n");

    // reading and convolving with input
    if (!convolve_input_file(input_path, rir, builder.sample_rate,
                             output_path)) {
      return 1;
    }
  }

  return 0;
}
