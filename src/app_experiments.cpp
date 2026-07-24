#include "Metrics.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// variance mode
static constexpr unsigned int kVarianceParticles = 100000;
static const std::vector<unsigned int> kVarianceRuns = {11, 12, 13, 14,
                                                        15, 17, 20};

// sweep mode
static const std::vector<unsigned int> kSweepParticleCounts = {
    1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
static constexpr unsigned int kNumRuns = 10;

static constexpr unsigned int kSeedStart = 1;

static const std::string kOutDir = "../output/experiments";

// one run for a given seed
struct RunResult {
  double rt60 = -1.0;
  double c50 = 0.0;
  std::vector<double> edc_norm; // EDC / EDC[0]
  bool has_receiver = false;
};

static RunResult simulate(double max_time, double dt,
                          unsigned int num_particles, unsigned int seed) {
  SimConfig cfg;
  cfg.num_particles = num_particles;
  cfg.dt = dt;
  cfg.max_time = max_time;
  cfg.deterministic = true;
  cfg.seed = seed;

  Scene room = make_standard();

  Atmosphere air;
  Simulation sim(room, cfg, air);
  sim.run_offline();

  RunResult r;
  if (sim.scene.receivers.empty())
    return r;
  r.has_receiver = true;

  const auto &hist = sim.scene.receivers[0].histogram;
  const double bw = Receiver::bin_width;

  std::vector<double> energy = metrics::broadband_energy(hist);
  std::vector<double> edc = metrics::energy_decay_curve(energy);

  r.rt60 = metrics::rt60(metrics::edc_db(edc), bw);
  r.c50 = metrics::clarity(energy, bw, 50.0);

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
    out << "run_index,seed,rt60,c50\n";

    std::printf("Variance test: %u runs at %u particles\n", nRuns,
                kVarianceParticles);

    for (unsigned int i = 0; i < nRuns; ++i) {
      unsigned int seed = kSeedStart + i;
      RunResult r = simulate(max_time, dt, kVarianceParticles, seed);
      out << i << "," << seed << "," << r.rt60 << "," << r.c50 << "\n";
      out.flush(); // ensures results are continuosly saved
      std::printf("| run %3u/%u\n", i + 1, nRuns);
    }

    std::printf("Wrote %s\n", path.c_str());
  }
  return 0;
}

static int run_sweep(double max_time, double dt) {
  const std::string summary_path = kOutDir + "/sweep_summary.csv";
  std::ofstream summary(summary_path);
  if (!summary) {
    std::printf("Failed to open %s\n", summary_path.c_str());
    return 1;
  }
  summary << "num_particles,avg_rt60,avg_c50,valid_rt60_runs\n";

  std::printf("Sweep with %u runs averaged each\n", kNumRuns);

  unsigned int seed = kSeedStart;
  for (unsigned int n : kSweepParticleCounts) {
    double rt60_sum = 0.0;
    unsigned int rt60_valid = 0;
    double c50_sum = 0.0;
    std::vector<double> edc_acc; // sum of normalised linear EDC curves

    // per-run scores for this particle count (same schema as the variance
    // files), so RT60/C50 spread per count can be shown as box plots
    const std::string runs_path =
        kOutDir + "/sweep_runs_" + std::to_string(n) + ".csv";
    std::ofstream runs(runs_path);
    if (!runs) {
      std::printf("Failed to open %s\n", runs_path.c_str());
      return 1;
    }
    runs << "run_index,seed,rt60,c50\n";

    for (unsigned int i = 0; i < kNumRuns; ++i, ++seed) {
      RunResult r = simulate(max_time, dt, n, seed);

      runs << i << "," << seed << "," << r.rt60 << "," << r.c50 << "\n";
      runs.flush();

      if (r.rt60 > 0.0) {
        rt60_sum += r.rt60;
        ++rt60_valid;
      }
      c50_sum += r.c50;

      if (r.edc_norm.size() > edc_acc.size())
        edc_acc.resize(r.edc_norm.size(),
                       0.0); // pad with 0s to tail
      for (std::size_t k = 0; k < r.edc_norm.size(); ++k)
        edc_acc[k] += r.edc_norm[k];

      std::printf("| n=%-7u run %2u/%u  rt60=%.3f\n", n, i + 1, kNumRuns,
                  r.rt60);
    }
    std::printf("| wrote %s\n", runs_path.c_str());

    double avg_rt60 = rt60_sum / rt60_valid;
    double avg_c50 = c50_sum / kNumRuns;
    summary << n << "," << avg_rt60 << "," << avg_c50 << "," << rt60_valid
            << "\n";
    summary.flush();

    // averaged EDC curve for this particle count
    const std::string edc_path = kOutDir + "/edc_" + std::to_string(n) + ".csv";
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

    std::printf("n=%u -> avg_rt60=%.3f s, avg_c50=%.2f dB\n", n, avg_rt60,
                avg_c50);
  }

  std::printf("Wrote %s\n", summary_path.c_str());
  return 0;
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
    return run_variance(20, 0.001);
  if (mode == "sweep")
    return run_sweep(20, 0.001);

  std::printf("Unknown mode '%s' (expected 'variance' or 'sweep')\n",
              mode.c_str());
  return 1;
}
