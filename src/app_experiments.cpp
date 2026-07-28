#include "Materials.hpp"
#include "Metrics.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// config mode: the two rows of the reference-vs-optimised comparison. The
// optimised values come from the sweeps; the other modes key off them.
static constexpr unsigned int kReferenceParticles = 1000000;
static constexpr unsigned int kReferenceDtMs = 1;
static constexpr unsigned int kOptimisedParticles = 200000;
static constexpr unsigned int kOptimisedDtMs = 20;

// variance mode: how many runs per set are needed for a stable spread
// estimate. Runs at the optimised config, since that is the configuration the
// reported results use — a run count justified at some other config would not
// transfer.
static constexpr unsigned int kVarianceParticles = kOptimisedParticles;
static constexpr unsigned int kVarianceDtMs = kOptimisedDtMs;
static const std::vector<unsigned int> kVarianceRuns = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 20};

// sweep mode: pick which variable the sweep walks over. The other one is held
// at its kSweepFixed* value below.
enum class SweepAxis { ParticleCount, TimeStep };
static constexpr SweepAxis kSweepAxis = SweepAxis::ParticleCount;

// swept when kSweepAxis == ParticleCount, fixed at kSweepFixedDt
static const std::vector<unsigned int> kSweepParticleCounts = {
    1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
static constexpr double kSweepFixedDt = 0.02;

// swept when kSweepAxis == TimeStep (in milliseconds), fixed at
// kSweepFixedParticles
static const std::vector<unsigned int> kSweepDtMs = {1, 2, 5, 10, 20, 50, 100};
static constexpr unsigned int kSweepFixedParticles = 200000;

static constexpr unsigned int kNumRuns = 10;

// edc mode: run at the optimised config, the point being that it reproduces
// the reference. Run once with the scene's own scattering and once forced to
// s = 0, which is the only setting comparable to a specular ISM baseline.
static constexpr unsigned int kEdcParticles = kOptimisedParticles;
static constexpr unsigned int kEdcDtMs = kOptimisedDtMs;

static constexpr unsigned int kSeedStart = 1;

static const std::string kOutDir = "../output/experiments";

// one run for a given seed
struct RunResult {
  double rt60 = -1.0;
  double c50 = 0.0;
  double runtime_ms = 0.0; // particle tracing only, no metric analysis
  std::array<double, kNumBands> rt60_bands{};
  std::vector<double> edc_norm; // EDC / EDC[0]
  bool has_receiver = false;
};

static RunResult simulate(double max_time, double dt,
                          unsigned int num_particles, unsigned int seed,
                          bool zero_scattering = false) {
  SimConfig cfg;
  cfg.num_particles = num_particles;
  cfg.dt = dt;
  cfg.max_time = max_time;
  cfg.deterministic = true;
  cfg.seed = seed;

  // Scene room = make_standard();
  Material room_material = materials::mSolidWood;
  Scene room = make_room(3, 4, 3, room_material);
  if (zero_scattering)
    for (Plane &plane : room.planes)
      plane.material.scattering.fill(0.0);

  Atmosphere air;

  // Times the simulation stage only: emission plus the tracing loop. Scene
  // construction and the metric analysis below are excluded so the number is
  // comparable across sweep points.
  const auto t_start = std::chrono::steady_clock::now();
  Simulation sim(room, cfg, air);
  sim.run_offline();
  const auto t_end = std::chrono::steady_clock::now();

  RunResult r;
  r.runtime_ms =
      std::chrono::duration<double, std::milli>(t_end - t_start).count();

  if (sim.scene.receivers.empty())
    return r;
  r.has_receiver = true;

  const auto &hist = sim.scene.receivers[0].histogram;
  const double bw = Receiver::bin_width;

  std::vector<double> energy = metrics::broadband_energy(hist);
  std::vector<double> edc = metrics::energy_decay_curve(energy);

  r.rt60 = metrics::rt60(metrics::edc_db(edc), bw);
  r.c50 = metrics::clarity(energy, bw, 50.0);

  // per-band RT60, for the band-wise Eyring-Norris comparison
  for (int b = 0; b < kNumBands; ++b) {
    std::vector<double> band_edc =
        metrics::energy_decay_curve(metrics::band_energy(hist, b));
    r.rt60_bands[b] = metrics::rt60(metrics::edc_db(band_edc), bw);
  }

  // normalised EDF to average curves across runs
  r.edc_norm.assign(edc.size(), 0.0);
  if (!edc.empty() && edc[0] > 0.0)
    for (std::size_t i = 0; i < edc.size(); ++i)
      r.edc_norm[i] = edc[i] / edc[0];

  return r;
}

static int run_variance(double max_time, double dt) {
  for (unsigned int nRuns : kVarianceRuns) {
    const std::string path =
        kOutDir + "/variance_" + std::to_string(nRuns) + ".csv";
    std::ofstream out(path);
    if (!out) {
      std::printf("Failed to open %s\n", path.c_str());
      return 1;
    }
    out << "run_index,seed,rt60,c50,runtime_ms\n";

    std::printf("Variance test: %u runs at %u particles\n", nRuns,
                kVarianceParticles);

    for (unsigned int i = 0; i < nRuns; ++i) {
      unsigned int seed = kSeedStart + i;
      RunResult r = simulate(max_time, dt, kVarianceParticles, seed);
      out << i << "," << seed << "," << r.rt60 << "," << r.c50 << ","
          << r.runtime_ms << "\n";
      out.flush(); // ensures results are continuosly saved
      std::printf("| run %3u/%u\n", i + 1, nRuns);
    }

    std::printf("Wrote %s\n", path.c_str());
  }
  return 0;
}

// One point of the sweep: the varying quantity resolved into concrete sim
// settings, plus the tag that names its output files.
struct SweepPoint {
  unsigned int axis_value; // particle count, or dt in ms
  unsigned int num_particles;
  double dt;
  std::string tag; // filename suffix: "200000" or "dt10"
};

static std::vector<SweepPoint> build_sweep_points() {
  std::vector<SweepPoint> points;
  if (kSweepAxis == SweepAxis::ParticleCount) {
    for (unsigned int n : kSweepParticleCounts)
      points.push_back({n, n, kSweepFixedDt, std::to_string(n)});
  } else {
    for (unsigned int dt_ms : kSweepDtMs)
      points.push_back({dt_ms, kSweepFixedParticles, dt_ms * 1e-3,
                        "dt" + std::to_string(dt_ms)});
  }
  return points;
}

static int run_sweep(double max_time) {
  const bool by_count = kSweepAxis == SweepAxis::ParticleCount;

  const std::string summary_path =
      kOutDir + (by_count ? "/sweep_summary.csv" : "/sweep_dt_summary.csv");
  std::ofstream summary(summary_path);
  if (!summary) {
    std::printf("Failed to open %s\n", summary_path.c_str());
    return 1;
  }
  summary << (by_count ? "num_particles" : "dt_ms")
          << ",avg_rt60,avg_c50,avg_runtime_ms,valid_rt60_runs\n";

  if (by_count)
    std::printf("Particle-count sweep at dt=%.3f ms, %u runs averaged each\n",
                kSweepFixedDt * 1e3, kNumRuns);
  else
    std::printf("dt sweep at %u particles, %u runs averaged each\n",
                kSweepFixedParticles, kNumRuns);

  for (const SweepPoint &pt : build_sweep_points()) {
    double rt60_sum = 0.0;
    unsigned int rt60_valid = 0;
    double c50_sum = 0.0;
    double runtime_sum = 0.0;
    std::vector<double> edc_acc; // sum of normalised linear EDC curves

    // per-run scores for this sweep point (same schema as the variance files),
    // so RT60/C50 spread per point can be shown as box plots
    const std::string runs_path = kOutDir + "/sweep_runs_" + pt.tag + ".csv";
    std::ofstream runs(runs_path);
    if (!runs) {
      std::printf("Failed to open %s\n", runs_path.c_str());
      return 1;
    }
    runs << "run_index,seed,rt60,c50,runtime_ms\n";

    // Common random numbers: every sweep point replays the same seed sequence,
    // so run i is measured from the same emitted particle set at every point.
    // Point-to-point differences are then far less seed-dependent than the
    // absolute spread within a point suggests.
    unsigned int seed = kSeedStart;
    for (unsigned int i = 0; i < kNumRuns; ++i, ++seed) {
      RunResult r = simulate(max_time, pt.dt, pt.num_particles, seed);

      runs << i << "," << seed << "," << r.rt60 << "," << r.c50 << ","
           << r.runtime_ms << "\n";
      runs.flush();

      if (r.rt60 > 0.0) {
        rt60_sum += r.rt60;
        ++rt60_valid;
      }
      c50_sum += r.c50;
      runtime_sum += r.runtime_ms;

      if (r.edc_norm.size() > edc_acc.size())
        edc_acc.resize(r.edc_norm.size(),
                       0.0); // pad with 0s to tail
      for (std::size_t k = 0; k < r.edc_norm.size(); ++k)
        edc_acc[k] += r.edc_norm[k];

      std::printf("| %-9s run %2u/%u  rt60=%.3f  %.0f ms\n", pt.tag.c_str(),
                  i + 1, kNumRuns, r.rt60, r.runtime_ms);
    }
    std::printf("| wrote %s\n", runs_path.c_str());

    double avg_rt60 = rt60_sum / rt60_valid;
    double avg_c50 = c50_sum / kNumRuns;
    double avg_runtime = runtime_sum / kNumRuns;
    summary << pt.axis_value << "," << avg_rt60 << "," << avg_c50 << ","
            << avg_runtime << "," << rt60_valid << "\n";
    summary.flush();

    // averaged EDC curve for this sweep point
    const std::string edc_path = kOutDir + "/edc_" + pt.tag + ".csv";
    std::ofstream edc_out(edc_path);
    if (edc_out) {
      edc_out << "time_ms,edc_db\n";
      const double lin_floor = 1e-12;
      for (std::size_t k = 0; k < edc_acc.size(); ++k) {
        double avg = edc_acc[k] / kNumRuns;
        double db = 10.0 * std::log10(avg > lin_floor ? avg : lin_floor);
        edc_out << (k * Receiver::bin_width * 1e3) << "," << db << "\n";
      }
      std::printf("| wrote %s\n", edc_path.c_str());
    }

    std::printf("%s -> avg_rt60=%.3f s, avg_c50=%.2f dB, avg_runtime=%.0f ms\n",
                pt.tag.c_str(), avg_rt60, avg_c50, avg_runtime);
  }

  std::printf("Wrote %s\n", summary_path.c_str());
  return 0;
}

// mean and sample standard deviation of a set of runs
static void mean_std(const std::vector<double> &v, double &mean, double &sd) {
  mean = 0.0;
  sd = 0.0;
  if (v.empty())
    return;
  for (double x : v)
    mean += x;
  mean /= v.size();
  if (v.size() < 2)
    return;
  for (double x : v)
    sd += (x - mean) * (x - mean);
  sd = std::sqrt(sd / (v.size() - 1));
}

// One row of the reference-vs-optimised comparison table.
static void run_config(double max_time, const std::string &label,
                       unsigned int num_particles, unsigned int dt_ms,
                       std::ofstream &summary) {
  std::vector<double> rt60s, c50s, runtimes;
  std::array<std::vector<double>, kNumBands> band_rt60s;

  std::printf("Config '%s': %u particles, dt=%u ms, %u runs\n", label.c_str(),
              num_particles, dt_ms, kNumRuns);

  for (unsigned int i = 0; i < kNumRuns; ++i) {
    RunResult r =
        simulate(max_time, dt_ms * 1e-3, num_particles, kSeedStart + i);
    if (r.rt60 > 0.0)
      rt60s.push_back(r.rt60);
    c50s.push_back(r.c50);
    runtimes.push_back(r.runtime_ms);
    for (int b = 0; b < kNumBands; ++b)
      if (r.rt60_bands[b] > 0.0)
        band_rt60s[b].push_back(r.rt60_bands[b]);

    std::printf("| run %2u/%u  rt60=%.3f  %.0f ms\n", i + 1, kNumRuns, r.rt60,
                r.runtime_ms);
  }

  double rt60_mean, rt60_sd, c50_mean, c50_sd, rt_mean, rt_sd;
  mean_std(rt60s, rt60_mean, rt60_sd);
  mean_std(c50s, c50_mean, c50_sd);
  mean_std(runtimes, rt_mean, rt_sd);

  summary << label << "," << num_particles << "," << dt_ms << "," << rt60_mean
          << "," << rt60_sd << "," << c50_mean << "," << c50_sd << ","
          << rt_mean << "," << rt_sd << "," << rt60s.size() << "\n";
  summary.flush();

  // per-band RT60 for this config, to sit next to Eyring-Norris per band
  const std::string bands_path =
      kOutDir + "/real-room/config_bands_" + label + ".csv";
  std::ofstream bands(bands_path);
  if (bands) {
    bands << "band_index,rt60_mean,rt60_std,valid_runs\n";
    for (int b = 0; b < kNumBands; ++b) {
      double m, sd;
      mean_std(band_rt60s[b], m, sd);
      bands << b << "," << m << "," << sd << "," << band_rt60s[b].size()
            << "\n";
    }
    std::printf("| wrote %s\n", bands_path.c_str());
  }

  std::printf("%s -> rt60=%.4f+-%.4f s, c50=%.2f+-%.2f dB, %.0f+-%.0f ms\n",
              label.c_str(), rt60_mean, rt60_sd, c50_mean, c50_sd, rt_mean,
              rt_sd);
}

static int run_configs(double max_time) {
  const std::string summary_path = kOutDir + "/real-room/config_summary.csv";
  std::ofstream summary(summary_path);
  if (!summary) {
    std::printf("Failed to open %s\n", summary_path.c_str());
    return 1;
  }
  summary << "label,num_particles,dt_ms,rt60_mean,rt60_std,c50_mean,c50_std,"
             "runtime_ms_mean,runtime_ms_std,valid_rt60_runs\n";

  run_config(max_time, "optimised", kOptimisedParticles, kOptimisedDtMs,
             summary);
  run_config(max_time, "reference", kReferenceParticles, kReferenceDtMs,
             summary);

  std::printf("Wrote %s\n", summary_path.c_str());
  return 0;
}

// Averaged EDC at one scattering setting, written as a dB curve.
static int write_edc(double max_time, const std::string &label,
                     bool zero_scattering) {
  std::vector<double> edc_acc;
  std::printf("EDC '%s': %u particles, dt=%u ms, %u runs, s=%s\n",
              label.c_str(), kEdcParticles, kEdcDtMs, kNumRuns,
              zero_scattering ? "0" : "scene");

  for (unsigned int i = 0; i < kNumRuns; ++i) {
    RunResult r = simulate(max_time, kEdcDtMs * 1e-3, kEdcParticles,
                           kSeedStart + i, zero_scattering);
    if (r.edc_norm.size() > edc_acc.size())
      edc_acc.resize(r.edc_norm.size(), 0.0);
    for (std::size_t k = 0; k < r.edc_norm.size(); ++k)
      edc_acc[k] += r.edc_norm[k];
    std::printf("| run %2u/%u  rt60=%.3f\n", i + 1, kNumRuns, r.rt60);
  }

  const std::string path = kOutDir + "/edc_" + label + ".csv";
  std::ofstream out(path);
  if (!out) {
    std::printf("Failed to open %s\n", path.c_str());
    return 1;
  }
  out << "time_ms,edc_db\n";
  const double lin_floor = 1e-12;
  for (std::size_t k = 0; k < edc_acc.size(); ++k) {
    double avg = edc_acc[k] / kNumRuns;
    double db = 10.0 * std::log10(avg > lin_floor ? avg : lin_floor);
    out << (k * Receiver::bin_width * 1e3) << "," << db << "\n";
  }
  std::printf("Wrote %s\n", path.c_str());
  return 0;
}

static int run_edc(double max_time) {
  if (int rc = write_edc(max_time, "specular", true))
    return rc;
  return write_edc(max_time, "scattering", false);
}

int main(int argc, char *argv[]) {
  std::string mode = (argc >= 2) ? argv[1] : "variance";

  std::error_code ec;
  std::filesystem::create_directories(kOutDir, ec);
  if (ec) {
    std::printf("Could not create %s: %s\n", kOutDir.c_str(),
                ec.message().c_str());
    return 1;
  }

  if (mode == "variance")
    return run_variance(20, kVarianceDtMs * 1e-3);
  if (mode == "sweep")
    return run_sweep(20);
  if (mode == "config")
    return run_configs(20);
  if (mode == "edc")
    return run_edc(20);

  std::printf(
      "Unknown mode '%s' (expected 'variance', 'sweep', 'config' or 'edc')\n",
      mode.c_str());
  return 1;
}
