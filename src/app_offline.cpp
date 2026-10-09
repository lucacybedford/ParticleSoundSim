#include "Bands.hpp"
#include "ConvolveInput.hpp"
#include "Convolver.hpp"
#include "Levels.hpp"
#include "Materials.hpp"
#include "RIRBuilder.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include "Wav.hpp"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

static std::vector<BandEnergies>
apply_power(const std::vector<BandEnergies> &hist, const Emitter &em) {
  std::vector<BandEnergies> out = hist;
  if (!em.source_power)
    return out;
  for (auto &bin : out)
    for (int b = 0; b < kNumBands; ++b)
      bin[b] *= (*em.source_power)[b];
  return out;
}

static std::vector<BandEnergies>
combine_histograms(const Receiver &rec, const std::vector<Emitter> &emitters) {
  std::vector<BandEnergies> combined;
  for (std::size_t e = 0; e < rec.histograms.size(); ++e) {
    auto hist = apply_power(rec.histograms[e], emitters[e]);
    if (hist.size() > combined.size())
      combined.resize(hist.size(), BandEnergies{});
    for (std::size_t t = 0; t < hist.size(); ++t)
      for (int b = 0; b < kNumBands; ++b)
        combined[t][b] += hist[t][b];
  }
  return combined;
}

static bool write_histogram_csv(const std::string &path,
                                const std::vector<BandEnergies> &hist,
                                double bin_width) {
  std::ofstream out(path);
  if (!out)
    return false;

  // enough digits to round-trip the analysis in the plotting scripts
  out << std::setprecision(10);

  out << "time_ms";
  for (int b = 0; b < kNumBands; ++b)
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

// app_offline [input.wav]
int main(int argc, char *argv[]) {
  SimConfig cfg;
  Atmosphere air;

  /*
  Configuration
  */

  enum class Room { Box, Cathedral, LivingRoom, CoupledRooms, Standard };
  const Room which = Room::Box;
  const bool standard = which == Room::Standard;

  cfg.num_particles = 200000;
  cfg.dt = 0.020;

  double room_width = 4;
  double room_length = 7;
  double room_height = 3;
  const Material &room_material = materials::mSolidWood;
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
    room = make_common_room();
    geometry = "living_room";
    break;
  case Room::CoupledRooms:
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

  // check all emitters have or do not have power
  size_t em_with_power = 0;
  for (const Emitter &em : room.emitters) {
    if (em.source_power)
      em_with_power++;
  }
  if (em_with_power > 0 && em_with_power < room.emitters.size())
    std::printf(
        "Some emitters do not have source power: inconsistent levels.\n");

  BandEnergies L_W;
  L_W.fill(70.0);
  room.emitters[0].set_power_dB(L_W);

  Simulation sim(room, cfg, air);

  std::printf("Speed of sound: %.2f m/s (T = %.1f C)\n", air.sound_speed(),
              air.temperature_c);

  std::printf("Particles: %i\n", cfg.num_particles);

  std::printf("Room type: %s\n", geometry.c_str());

  sim.run_offline();

  std::printf("Offline run finished at t = %.3f s, %zu particles still alive\n",
              sim.time, sim.particles.size());

  bool all_powered = true;
  for (const Emitter &em : sim.scene.emitters)
    all_powered = all_powered && em.source_power.has_value();
  const double rho_c = air.air_density() * air.sound_speed();

  // save receiver information to csv file
  for (std::size_t i = 0; i < sim.scene.receivers.size(); ++i) {
    const auto &rec = sim.scene.receivers[i];
    const auto hist = combine_histograms(rec, sim.scene.emitters);

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

    // steady-state SPL for constant sources (needs every emitter's power)
    if (all_powered && total > 0) {
      const BandEnergies I = levels::steady_intensity(hist, rec.size);
      const BandEnergies L = levels::band_spl(I, rho_c);
      std::printf("  SPL (dB re 20 uPa, constant source):");
      for (int b = 0; b < kNumBands; ++b)
        std::printf(" %g Hz %.1f%s", kBandCentres[b], L[b],
                    b + 1 < kNumBands ? "," : "\n");
      std::printf("  Overall SPL: %.1f dB\n", levels::overall_spl(I, rho_c));
    }

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

  std::vector<std::string> dry_paths(argv + 1, argv + argc);
  if (dry_paths.empty())
    dry_paths.push_back("dry.wav");

  const std::size_t num_emitters = sim.scene.emitters.size();
  if (dry_paths.size() > num_emitters)
    std::printf("%zu dry files given for %zu emitters, ignoring the extras\n",
                dry_paths.size(), num_emitters);
  const std::string last = dry_paths.back();
  dry_paths.resize(num_emitters, last);

  for (std::size_t e = 0; e < num_emitters; ++e)
    std::printf("Emitter %zu <- %s\n", e, dry_paths[e].c_str());

  RIRBuilder builder;
  builder.bin_width = Receiver::bin_width;

  std::vector<std::vector<float>> dry(num_emitters);
  for (std::size_t e = 0; e < num_emitters; ++e) {
    if (!load_dry(dry_paths[e], builder.sample_rate, dry[e]))
      continue;
    if (sim.scene.emitters[e].source_power)
      normalise_rms(dry[e]);
  }

  std::string stem;
  if (standard) {
    stem = "standard/standard-room-" + r_particles;
  } else {
    stem = r_particles + "_" + geometry;
  }

  // one RIR per (receiver, emitter) pair, convolved with that emitter's dry
  // signal; the wet signals are summed so relative source levels are kept
  for (std::size_t r = 0; r < sim.scene.receivers.size(); ++r) {
    const Receiver &rec = sim.scene.receivers[r];
    std::vector<float> mix;

    for (std::size_t e = 0; e < num_emitters; ++e) {
      const std::string tag =
          "_r" + std::to_string(r) + "_e" + std::to_string(e);

      // independent noise carrier per pair, so sources don't correlate
      builder.seed = 1234 + static_cast<unsigned>(r * num_emitters + e);
      std::vector<float> rir =
          builder.build(apply_power(rec.histograms[e], sim.scene.emitters[e]));
      if (rir.empty()) {
        std::printf("Receiver %zu, emitter %zu: no energy arrived.\n", r, e);
        continue;
      }

      const std::string rir_path = "../output/" + stem + "_rir" + tag + ".wav";
      Audio rir_audio{builder.sample_rate, rir};
      if (!wav_write(rir_path, rir_audio)) {
        std::printf("Failed to write %s (directory must exist)\n",
                    rir_path.c_str());
        return 1;
      }
      std::printf("Wrote %s (%zu samples, %.3f s)\n", rir_path.c_str(),
                  rir.size(),
                  rir.size() / static_cast<double>(builder.sample_rate));

      // default input for ParticleSoundSimConvolve
      if (r == 0 && e == 0 && !wav_write("rir.wav", rir_audio))
        std::printf("Failed to write rir.wav\n");

      if (dry[e].empty())
        continue; // dry file is missing, nothing to convolve

      std::vector<float> wet = convolve(dry[e], rir);

      if (wet.size() > mix.size())
        mix.resize(wet.size(), 0.0f);
      for (std::size_t i = 0; i < wet.size(); ++i)
        mix[i] += wet[i];

      // per-source listening copy, normalised on its own
      normalize_peak(wet);
      wav_write("../output/" + stem + tag + ".wav",
                Audio{builder.sample_rate, wet});
    }

    if (mix.empty())
      continue;

    // normalise the sum only, so relative source levels survive
    normalize_peak(mix);
    const std::string output_path =
        "../output/" + stem + "_" + std::to_string(r) + ".wav";
    if (!wav_write(output_path, Audio{builder.sample_rate, mix})) {
      std::printf("Failed to write %s (directory must exist)\n",
                  output_path.c_str());
      return 1;
    }
    std::printf("Wrote %s\n", output_path.c_str());
  }
  return 0;
}
