#include "Convolver.hpp"
#include "Materials.hpp"
#include "RIRBuilder.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include "Wav.hpp"
#include <cstdio>
#include <string>

int main(int argc, char *argv[]) {
  SimConfig cfg;
  Atmosphere air;

  cfg.num_particles = 100000;

  float room_width = 5;
  float room_length = 10;
  float room_height = 3;
  Material room_material = materials::mConcrete;

  Simulation sim(make_room(room_width, room_length, room_height, room_material),
                 cfg, air);
  // Simulation sim(make_L_room(room_material), cfg, air);

  if (cfg.dt > Receiver::bin_width) {
    printf("dt must be smaller than receiver bin width.");
    return 1;
  }

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

  std::string r_width = std::to_string(static_cast<int>(room_width));
  std::string r_height = std::to_string(static_cast<int>(room_height));
  std::string r_material = room_material.name;
  std::string r_particles = std::to_string(cfg.num_particles);

  std::string input_path = "dry.wav";
  if (argc == 2) {
    input_path = argv[1];
  }
  std::string output_path = "./output/3D/3d-new-" + r_width + "x" + r_height +
                            "_" + r_material + "_room_" + r_particles + ".wav";

  if (!sim.scene.receivers.empty()) {
    RIRBuilder builder;
    builder.sample_rate = 44100;
    builder.bin_width = Receiver::bin_width;

    std::vector<float> rir = builder.build(sim.scene.receivers[0].histogram);
    if (rir.empty()) {
      std::printf("RIR is empty (no energy reached the receiver).\n");
      return 0;
    }

    Audio rir_audio{builder.sample_rate, rir};
    normalize_peak(rir_audio.samples);
    wav_write("rir.wav", rir_audio);
    std::printf("Wrote rir.wav (%zu samples, %.3f s)\n", rir.size(),
                rir.size() / static_cast<double>(builder.sample_rate));

    Audio dry;
    if (wav_read(input_path, dry)) {
      if (dry.sample_rate != builder.sample_rate) {
        std::printf("Resampling %s from %d Hz to %d Hz\n", input_path.c_str(),
                    dry.sample_rate, builder.sample_rate);
        dry.samples =
            resample(dry.samples, dry.sample_rate, builder.sample_rate);
        dry.sample_rate = builder.sample_rate;
        if (dry.samples.empty()) {
          std::printf("Resampling failed – skipping convolution.\n");
          return 1;
        }
      }
      std::vector<float> wet = convolve(dry.samples, rir);
      normalize_peak(wet);
      wav_write(output_path, Audio{dry.sample_rate, wet});
      std::printf("Convolved %s -> wet.wav (%zu samples)\n", input_path.c_str(),
                  wet.size());
    } else {
      std::printf("No %s found – skipping convolution.\n", input_path.c_str());
    }
  }

  return 0;
}
