#pragma once
#include <vector>

// FFT-based overlap-add linear convolution. Computes y = x * h where x is a dry
// signal and h is an impulse response. The result has length x.size()+h.size()-1.
//
// Why overlap-add: direct time-domain convolution costs O(N*M), which is brutal
// for room-length impulse responses (tens of thousands of taps). We instead cut
// x into blocks, FFT-convolve each block with h (multiplication in the frequency
// domain), and add the overlapping tails back together. h is transformed once
// and reused for every block.
std::vector<float> convolve(const std::vector<float> &x,
                            const std::vector<float> &h);
