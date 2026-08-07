#include "Convolver.hpp"
#include "Materials.hpp"
#include "Metrics.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

enum class RoomChoice { Standard, Real };
static constexpr RoomChoice kRoom = RoomChoice::Real;

// geometry and material of the Real room (Standard is fixed in make_standard)
static constexpr float kRealWidth = 4.0f;
static constexpr float kRealLength = 7.0f;
static constexpr float kRealHeight = 3.0f;
static const Material &kRealMaterial = materials::mSolidWood;

static Scene build_room() {
  if (kRoom == RoomChoice::Standard)
    return make_standard();
  Material material = kRealMaterial;
  return make_room(kRealWidth, kRealLength, kRealHeight, material);
}

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

// Sentinel meaning "leave each surface's own scattering coefficient alone".
// Any value in [0,1] passed to simulate() overrides every surface instead.
static constexpr double kSceneScattering = -1.0;

// scatter mode: per-band RT60 at the optimised config across scattering
// settings. The residual in the per-band Eyring comparison grows with
// absorption; if it is caused by the field being insufficiently diffuse (so
// that the incidence-angle distribution departs from random incidence, and
// Eyring's own diffuse-field assumption weakens), forcing s upward should
// shrink it. If it is unchanged, the cause lies elsewhere.
static const std::vector<std::pair<std::string, double>> kScatterSettings = {
    {"s000", 0.0}, {"scene", kSceneScattering}, {"s050", 0.5}, {"s100", 1.0}};

// convolve mode: cost of the convolution stage against dry-input length, at a
// standardised RIR length. This is a pure DSP benchmark with no simulation in
// it, so it is room-independent and always writes to the top-level experiment
// directory whatever kRoom is set to.
static constexpr int kConvSampleRate = 44100; // RIRBuilder's rate
static constexpr unsigned int kConvSeed = 1;
static constexpr unsigned int kConvWarmups = 1;
static constexpr unsigned int kConvRepeats = 10;

// Standardised RIR length for the input sweep. 0.5 s covers the measured decay
// of both rooms (0.33 s and 0.49 s) with margin, and is a stated constant
// rather than whatever the last simulation happened to produce.
static constexpr double kConvRirSeconds = 0.5;

// Input lengths. The short end matters: overlap-add consumes the input in
// blocks of nfft - M + 1 samples (~0.99 s at a 0.5 s RIR), so everything below
// one block costs the same and the curve is a staircase, not a line. Sampling
// only the long end would hide that.
static const std::vector<double> kConvInputSeconds = {
    0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 60.0};

// Secondary sweep: RIR length at a fixed input, to show that the standardised
// 0.5 s is not a special operating point. nfft doubles with the RIR, but so
// does the block advance, so cost per second of audio moves far less than the
// RIR length does.
static constexpr double kConvRirSweepInputSeconds = 10.0;
static const std::vector<double> kConvRirSweepSeconds = {0.25, 0.5, 1.0, 2.0};

static constexpr unsigned int kSeedStart = 1;

static const std::string kOutDir = kRoom == RoomChoice::Standard
                                       ? "../output/experiments"
                                       : "../output/experiments/real-room";

// convolve mode only — see kConvRirSeconds for why this ignores kRoom
static const std::string kConvOutDir = "../output/experiments";

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
                          double scattering_override = kSceneScattering) {
  SimConfig cfg;
  cfg.num_particles = num_particles;
  cfg.dt = dt;
  cfg.max_time = max_time;
  cfg.deterministic = true;
  cfg.seed = seed;

  Scene room = build_room();
  if (scattering_override != kSceneScattering)
    for (Plane &plane : room.planes)
      plane.material.scattering.fill(scattering_override);

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

// One row of the reference-vs-optimised comparison table, and the same routine
// reused for one setting of the scatter sweep.
static void run_config(double max_time, const std::string &label,
                       unsigned int num_particles, unsigned int dt_ms,
                       std::ofstream &summary,
                       double scattering = kSceneScattering) {
  std::vector<double> rt60s, c50s, runtimes;
  std::array<std::vector<double>, kNumBands> band_rt60s;

  std::printf("Config '%s': %u particles, dt=%u ms, %u runs, s=", label.c_str(),
              num_particles, dt_ms, kNumRuns);
  if (scattering == kSceneScattering)
    std::printf("scene\n");
  else
    std::printf("%.2f\n", scattering);

  for (unsigned int i = 0; i < kNumRuns; ++i) {
    RunResult r = simulate(max_time, dt_ms * 1e-3, num_particles,
                           kSeedStart + i, scattering);
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
  const std::string bands_path = kOutDir + "/config_bands_" + label + ".csv";
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

// Per-band RT60 at the optimised config across a range of scattering
// settings, to test whether the frequency-dependent residual against
// Eyring-Norris is driven by how diffuse the field is.
static int run_scatter(double max_time) {
  const std::string summary_path = kOutDir + "/scatter_summary.csv";
  std::ofstream summary(summary_path);
  if (!summary) {
    std::printf("Failed to open %s\n", summary_path.c_str());
    return 1;
  }
  summary << "label,num_particles,dt_ms,rt60_mean,rt60_std,c50_mean,c50_std,"
             "runtime_ms_mean,runtime_ms_std,valid_rt60_runs\n";

  for (const auto &setting : kScatterSettings)
    run_config(max_time, setting.first, kOptimisedParticles, kOptimisedDtMs,
               summary, setting.second);

  std::printf("Wrote %s\n", summary_path.c_str());
  return 0;
}

static int run_configs(double max_time) {
  const std::string summary_path = kOutDir + "/config_summary.csv";
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
                     double scattering) {
  std::vector<double> edc_acc;
  std::printf("EDC '%s': %u particles, dt=%u ms, %u runs, s=%s\n",
              label.c_str(), kEdcParticles, kEdcDtMs, kNumRuns,
              scattering == kSceneScattering ? "scene" : "0");

  for (unsigned int i = 0; i < kNumRuns; ++i) {
    RunResult r = simulate(max_time, kEdcDtMs * 1e-3, kEdcParticles,
                           kSeedStart + i, scattering);
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
  if (int rc = write_edc(max_time, "specular", 0.0))
    return rc;
  return write_edc(max_time, "scattering", kSceneScattering);
}

// ---------------------------------------------------------------- convolve

// White noise in [-1, 1], deterministic. The convolver's cost is independent
// of signal content (pocketfft has no data-dependent branches), but silence
// would risk denormal handling in the spectral multiply, so noise is the
// honest choice of filler.
static std::vector<float> conv_noise(std::size_t n, unsigned int seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::vector<float> x(n);
  for (float &v : x)
    v = dist(rng);
  return x;
}

// Synthetic RIR: noise under an envelope decaying 60 dB across its own length.
// Used rather than a simulated rir.wav so the benchmark reproduces without a
// prior run — cost depends on the RIR's length alone, not its contents.
static std::vector<float> conv_rir(double seconds, unsigned int seed) {
  const std::size_t n =
      static_cast<std::size_t>(seconds * kConvSampleRate + 0.5);
  std::vector<float> h = conv_noise(n, seed);
  for (std::size_t i = 0; i < n; ++i)
    h[i] *= static_cast<float>(
        std::exp(-6.907755 * static_cast<double>(i) / static_cast<double>(n)));
  return h;
}

static double conv_median(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  const std::size_t n = v.size();
  if (n == 0)
    return 0.0;
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// One sweep: each point is (input seconds, RIR seconds). Times convolve()
// alone — signals are built and resident in memory first, and no WAV read,
// resample or write is inside the timed region. That is the whole point of
// running this here rather than through ParticleSoundSimConvolve, which times
// file I/O along with the arithmetic.
static int
run_convolve_points(const std::string &path,
                    const std::vector<std::pair<double, double>> &points) {
  std::ofstream out(path);
  if (!out) {
    std::printf("Failed to open %s\n", path.c_str());
    return 1;
  }
  // One row per point: the median over the repeats, and the two normalised
  // forms of it. The individual repeat times are not kept — they exist only
  // to make the median robust to a scheduler hiccup in any one call.
  out << "input_seconds,rir_seconds,median_ms,ms_per_second_audio,rtf\n";

  std::printf("%12s %10s %10s %12s %8s\n", "input (s)", "rir (s)", "med (ms)",
              "ms/s audio", "RTF");

  // Keeps the compiler from discarding the convolution as dead work.
  double sink = 0.0;

  for (const auto &pt : points) {
    const double in_secs = pt.first;
    const double rir_secs = pt.second;
    const std::size_t in_n =
        static_cast<std::size_t>(in_secs * kConvSampleRate + 0.5);

    const std::vector<float> x = conv_noise(in_n, kConvSeed);
    const std::vector<float> h = conv_rir(rir_secs, kConvSeed + 1);

    for (unsigned int w = 0; w < kConvWarmups; ++w) {
      std::vector<float> y = convolve(x, h);
      sink += y.empty() ? 0.0 : y[y.size() / 2];
    }

    std::vector<double> times;
    times.reserve(kConvRepeats);
    for (unsigned int i = 0; i < kConvRepeats; ++i) {
      const auto t0 = std::chrono::steady_clock::now();
      std::vector<float> y = convolve(x, h);
      const auto t1 = std::chrono::steady_clock::now();
      sink += y.empty() ? 0.0 : y[y.size() / 2];

      times.push_back(
          std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    const double med = conv_median(times);
    const double per_second = med / in_secs;
    const double rtf = in_secs * 1e3 / med; // seconds of audio per second

    out << in_secs << "," << rir_secs << "," << med << "," << per_second << ","
        << rtf << "\n";
    out.flush();

    std::printf("%12.2f %10.2f %10.2f %12.2f %8.0f\n", in_secs, rir_secs, med,
                per_second, rtf);
  }

  std::printf("Wrote %s (checksum %.6f)\n", path.c_str(), sink);
  return 0;
}

static int run_convolve() {
  std::error_code ec;
  std::filesystem::create_directories(kConvOutDir, ec);
  if (ec) {
    std::printf("Could not create %s: %s\n", kConvOutDir.c_str(),
                ec.message().c_str());
    return 1;
  }

  std::printf("Convolution timing: %u repeats after %u warm-up, %d Hz\n\n",
              kConvRepeats, kConvWarmups, kConvSampleRate);

  std::printf("Input-length sweep at a %.2f s RIR\n", kConvRirSeconds);
  std::vector<std::pair<double, double>> input_sweep;
  for (double s : kConvInputSeconds)
    input_sweep.emplace_back(s, kConvRirSeconds);
  if (int rc = run_convolve_points(kConvOutDir + "/convolve_input_sweep.csv",
                                   input_sweep))
    return rc;

  std::printf("\nRIR-length sweep at a %.2f s input\n",
              kConvRirSweepInputSeconds);
  std::vector<std::pair<double, double>> rir_sweep;
  for (double s : kConvRirSweepSeconds)
    rir_sweep.emplace_back(kConvRirSweepInputSeconds, s);
  return run_convolve_points(kConvOutDir + "/convolve_rir_sweep.csv",
                             rir_sweep);
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
  if (mode == "scatter")
    return run_scatter(20);
  if (mode == "convolve")
    return run_convolve();

  std::printf(
      "Unknown mode '%s' (expected 'variance', 'sweep', 'config', 'edc', "
      "'scatter' or 'convolve')\n",
      mode.c_str());
  return 1;
}
