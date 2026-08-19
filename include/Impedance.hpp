#pragma once
#include "Bands.hpp"

namespace impedance {

double alpha_random(double xi); // random-averaging absorption from impedance

double calibrate_impedance(double alpha);

BandEnergies calibrate(const BandEnergies &absorption);

} // namespace impedance
