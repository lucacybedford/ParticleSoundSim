#pragma once
#include <string>
#include <vector>

bool convolve_input_file(const std::string &input_path,
                         const std::vector<float> &rir, int rir_sample_rate,
                         const std::string &output_path);
