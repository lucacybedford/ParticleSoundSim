#pragma once
#include <string>
#include <vector>

struct Audio {
  int sample_rate = 44100;
  std::vector<float> samples; // mono, in [-1, 1]
};

// Reads a 16-bit PCM WAV. Multi-channel files are downmixed to mono.
// Returns false if the file is missing or not 16-bit PCM.
bool wav_read(const std::string &path, Audio &out);

// Writes a 16-bit PCM mono WAV. Samples outside [-1, 1] are clipped.
bool wav_write(const std::string &path, const Audio &in);

// Scales samples in place so the peak magnitude equals `peak` (default 0.9).
void normalize_peak(std::vector<float> &samples, float peak = 0.9f);
