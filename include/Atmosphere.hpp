#pragma once

struct Atmosphere {
  double temperature_c = 20.0;   // degrees Celsius
  double humidity = 50.0;        // % relative humidity
  double pressure_kpa = 101.325; // atmospheric pressure

  double sound_speed() const;

  // ISO 9613-1 pure-tone atmospheric absorption at frequency f
  double absorption_dB_per_m(double f) const;
  double absorption_m(double f) const;
};
