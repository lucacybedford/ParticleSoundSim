#pragma once
#include <array>

// The simulation is frequency-resolved over these ISO/ANSI octave bands.
// Everything that stores per-band data (particle energies, surface
// absorption, receiver histograms, air absorption, RIR synthesis) sizes
// itself from kNumBands, so extending the band range is a one-file change.

inline constexpr int kNumBands = 8;

// ANSI S1.6 band numbers; centre frequency = 10^(n/10) Hz
inline constexpr std::array<int, kNumBands> kBandNumbers{18, 21, 24, 27,
                                                         30, 33, 36, 39};

// Nominal octave-band centre frequencies in Hz (63 Hz .. 8 kHz)
inline constexpr std::array<double, kNumBands> kBandCentres{
    63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0};

// Per-band energy (or absorption-coefficient) vector
using BandEnergies = std::array<double, kNumBands>;
