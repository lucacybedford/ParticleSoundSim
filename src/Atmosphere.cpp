#include "Atmosphere.hpp"
#include <cmath>

double Atmosphere::sound_speed() const {
  constexpr double c0 = 343.2; // m/s at 20 C
  return c0 * std::sqrt((273.15 + temperature_c) / 293.15);
}

// ISO 9613-1 atmospheric absorption
double Atmosphere::absorption_dB_per_m(double f) const {
  constexpr double pr = 101.325; // reference pressure, kPa
  constexpr double T0 = 293.15;  // reference temperature, K (20 C)
  constexpr double T01 = 273.16; // triple-point isotherm, K

  const double T = temperature_c + 273.15;
  const double pa_pr = pressure_kpa / pr;

  // molar concentration of water vapour in air (%)
  const double psat_pr =
      std::pow(10.0, -6.8346 * std::pow(T01 / T, 1.261) + 4.6151);
  const double h = humidity * psat_pr / pa_pr;

  // oxygen and nitrogen relaxation frequencies (Hz)
  const double frO = pa_pr * (24.0 + 4.04e4 * h * (0.02 + h) / (0.391 + h));
  const double frN =
      pa_pr * std::pow(T / T0, -0.5) *
      (9.0 +
       280.0 * h * std::exp(-4.170 * (std::pow(T / T0, -1.0 / 3.0) - 1.0)));

  const double f2 = f * f;
  const double alpha =
      8.686 * f2 *
      (1.84e-11 / pa_pr * std::sqrt(T / T0) +
       std::pow(T / T0, -2.5) *
           (0.01275 * std::exp(-2239.1 / T) / (frO + f2 / frO) +
            0.1068 * std::exp(-3352.0 / T) / (frN + f2 / frN)));

  return alpha; // attenuation coefficient in dB/m
}

double Atmosphere::absorption_m(double f) const {
  // turning attenuation coefficient into energy absorption coefficient
  return absorption_dB_per_m(f) / 4.343;
}
