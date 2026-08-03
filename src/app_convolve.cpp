#include "ConvolveInput.hpp"
#include "Wav.hpp"
#include <chrono>
#include <cstdio>
#include <string>

// app_convolve [rir.wav] [input.wav] [output.wav]
int main(int argc, char *argv[]) {
  std::string rir_path = argc > 1 ? argv[1] : "rir.wav";
  std::string input_path = argc > 2 ? argv[2] : "dry.wav";
  std::string output_path = argc > 3 ? argv[3] : "wet.wav";

  Audio rir;
  if (!wav_read(rir_path, rir)) {
    std::printf("Failed to read RIR %s\n", rir_path.c_str());
    return 1;
  }
  std::printf("Loaded RIR %s (%zu samples, %d Hz)\n", rir_path.c_str(),
              rir.samples.size(), rir.sample_rate);

  auto t0 = std::chrono::steady_clock::now();
  bool ok = convolve_input_file(input_path, rir.samples, rir.sample_rate,
                                output_path);
  auto t1 = std::chrono::steady_clock::now();

  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
  std::printf("Reading and convolution took %.2f ms\n", ms);

  return ok ? 0 : 1;
}
