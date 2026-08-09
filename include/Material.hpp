#pragma once
#include "Bands.hpp"
#include <string>

struct Material {
  std::string name;
  BandEnergies absorption;
  BandEnergies impedance;
  // One coefficient for the whole spectrum, not a per-band curve. A particle
  // carries every band along a single trajectory, so the one reflection
  // decision at a contact has to serve all eight bands at once and a per-band
  // coefficient cannot be honoured. Spreading s across bands and collapsing it
  // again at each contact only produces an effective coefficient that drifts
  // downward as the high bands are absorbed, which models nothing.
  double scattering;
};
