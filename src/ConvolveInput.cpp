#include "ConvolveInput.hpp"
#include "Convolver.hpp"
#include "Wav.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

bool convolve_input_file(const std::string &input_path,
                         const std::vector<float> &rir, int rir_sample_rate,
                         const std::string &output_path) {
  if (rir.empty()) {
    std::printf("RIR is empty – skipping convolution.\n");
    return false;
  }

  Audio dry;
  if (!wav_read(input_path, dry)) {
    std::printf("No %s found, skipping convolution.\n", input_path.c_str());
    return true; // absence of input is not an error
  }

  if (dry.sample_rate != rir_sample_rate) {
    std::printf("Resampling %s from %d Hz to %d Hz\n", input_path.c_str(),
                dry.sample_rate, rir_sample_rate);
    dry.samples = resample(dry.samples, dry.sample_rate, rir_sample_rate);
    dry.sample_rate = rir_sample_rate;
    if (dry.samples.empty()) {
      std::printf("Resampling failed, skipping convolution.\n");
      return false;
    }
  }

  std::vector<float> wet = convolve(dry.samples, rir);
  float peak = 0.0f;
  for (float v : wet)
    peak = std::max(peak, std::fabs(v));

  if (!wav_write(output_path, Audio{dry.sample_rate, wet})) {
    std::printf("Failed to write %s (directory must exist)\n",
                output_path.c_str());
    return false;
  }

  std::printf("Convolved %s -> %s\n", input_path.c_str(), output_path.c_str());
  return true;
}
