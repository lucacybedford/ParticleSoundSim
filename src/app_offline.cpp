#include "Convolver.hpp"
#include "Materials.hpp"
#include "RIRBuilder.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include "Wav.hpp"
#include <cstdio>

int main() {
  SimConfig cfg;
  Atmosphere air;

  float room_width = 5;
  float room_height = 10;
  Material room_material = materials::mSolidWood;

  Simulation sim(make_room(room_width, room_height, room_material), cfg, air);

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

  std::string input_path = "dry.wav";
  std::string output_path = "./output/" +
                            std::to_string(static_cast<int>(room_width)) + "x" +
                            std::to_string(static_cast<int>(room_height)) +
                            "_" + room_material.name + "_room_100000.wav";

  // Build a pressure RIR from the first receiver and export it. Convolve a dry
  // signal with it if a dry.wav is sitting in the working directory.
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
      // (For a real run, resample dry to the RIR rate if they differ.)
      std::vector<float> wet = convolve(dry.samples, rir);
      normalize_peak(wet);
      wav_write(output_path, Audio{dry.sample_rate, wet});
      std::printf("Convolved dry.wav -> wet.wav (%zu samples)\n", wet.size());
    } else {
      std::printf("No dry.wav found -- skipping convolution.\n");
    }
  }

  return 0;
}
