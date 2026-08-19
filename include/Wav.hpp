#pragma once
#include <string>
#include <vector>

struct Audio {
  int sample_rate = 44100;
  std::vector<float> samples; // in [-1, 1]
};

// reads a WAV in any format dr_wav supports
bool wav_read(const std::string &path, Audio &out);

// writes a WAV with absolute sample values preserved exactly
bool wav_write(const std::string &path, const Audio &in);

// old peak normalisation
void normalize_peak(std::vector<float> &samples, float peak = 0.9f);

// resamples audio to out_rate
std::vector<float> resample(const std::vector<float> &in, int in_rate,
                            int out_rate);
