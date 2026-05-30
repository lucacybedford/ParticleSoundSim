#pragma once

// The medium the sound travels through. Temperature (with humidity/pressure
// later) determines both the speed of sound and the ISO air-absorption
// coefficients, so they are grouped here rather than scattered as constants.
//
// This is shared, accurate physics: BOTH the offline and visual apps use the
// same Atmosphere. Whether the visual app then *approximates* the absorption
// (e.g. one band instead of eight) is a separate Fidelity choice in SimConfig.
struct Atmosphere {
  double temperature_c = 20.0;   // degrees Celsius
  double humidity = 50.0;        // % relative humidity (for air absorption, later)
  double pressure_kpa = 101.325; // ambient pressure (for air absorption, later)

  // Speed of sound in m/s (== units/s, since 1 unit = 1 metre).
  // Rindel (2024): c = c0 * sqrt((273.15 + T) / 293.15), c0 = 343.2 m/s at 20 C.
  double sound_speed() const;

  // ISO 9613-1 pure-tone atmospheric absorption at frequency f (Hz).
  double absorption_dB_per_m(double f) const; // SPL attenuation, dB/m
  double absorption_m(double f) const;        // energy coefficient, 1/m (Np/m)
};
