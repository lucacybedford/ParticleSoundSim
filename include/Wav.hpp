#pragma once
#include <string>
#include <vector>

struct Audio {
  int sample_rate = 44100;
  std::vector<float> samples; // mono, in [-1, 1]
};

// Reads a WAV in any format dr_wav supports (8/16/24/32-bit PCM, float,
// ADPCM). Multi-channel files are downmixed to mono. Returns false if the
// file is missing or unreadable.
bool wav_read(const std::string &path, Audio &out);

// Writes a mono 32-bit IEEE float WAV: no clipping, absolute sample values
// are preserved exactly.
bool wav_write(const std::string &path, const Audio &in);

// Scales samples in place so the peak magnitude equals `peak` (default 0.9).
void normalize_peak(std::vector<float> &samples, float peak = 0.9f);

// Resamples mono audio to out_rate via libsamplerate (best-quality sinc).
// Returns an empty vector on conversion failure.
std::vector<float> resample(const std::vector<float> &in, int in_rate,
                            int out_rate);
