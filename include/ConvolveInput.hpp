#pragma once
#include <string>
#include <vector>

bool convolve_input_file(const std::string &input_path,
                         const std::vector<float> &rir, int rir_sample_rate,
                         const std::string &output_path);

// reads a WAV file and resamples it to target_rate
bool load_dry(const std::string &path, int target_rate,
              std::vector<float> &out);
