#pragma once
#include <array>

// inline for keeping a single instance
// constexpr for it to exist at compile time
inline constexpr int kNumBands = 8;

// ANSI band numbers
inline constexpr std::array<int, kNumBands> kBandNumbers{18, 21, 24, 27,
                                                         30, 33, 36, 39};

// octave-band centre frequencies in Hz
inline constexpr std::array<double, kNumBands> kBandCentres{
    63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0};

// per-band energy (or absorption-coefficient) vector
using BandEnergies = std::array<double, kNumBands>;
