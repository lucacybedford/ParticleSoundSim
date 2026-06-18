#pragma once
#include "Bands.hpp"
#include <string>

struct Material {
  std::string name;
  BandEnergies absorption;
};
