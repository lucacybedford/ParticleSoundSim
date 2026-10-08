#include "ConvolveInput.hpp"
#include "Convolver.hpp"
#include "Wav.hpp"
#include <cstdio>
#include <utility>

bool load_dry(const std::string &path, int target_rate,
              std::vector<float> &out) {
  Audio dry;
  if (!wav_read(path, dry)) {
    std::printf("No %s found.\n", path.c_str());
    return false;
  }

  if (dry.sample_rate != target_rate) {
    std::printf("Resampling %s from %d Hz to %d Hz\n", path.c_str(),
                dry.sample_rate, target_rate);
    dry.samples = resample(dry.samples, dry.sample_rate, target_rate);
    if (dry.samples.empty()) {
      std::printf("Resampling failed.\n");
      return false;
    }
  }

  out = std::move(dry.samples);
  return true;
}

bool convolve_input_file(const std::string &input_path,
                         const std::vector<float> &rir, int rir_sample_rate,
                         const std::string &output_path) {
  if (rir.empty()) {
    std::printf("RIR is empty – skipping convolution.\n");
    return false;
  }

  std::vector<float> dry;
  if (!load_dry(input_path, rir_sample_rate, dry)) {
    std::printf("Skipping convolution.\n");
    return true;
  }

  std::vector<float> wet = convolve(dry, rir);

  if (!wav_write(output_path, Audio{rir_sample_rate, wet})) {
    std::printf("Failed to write %s (directory must exist)\n",
                output_path.c_str());
    return false;
  }

  std::printf("Convolved %s -> %s\n", input_path.c_str(), output_path.c_str());
  return true;
}
