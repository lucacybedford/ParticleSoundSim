#include "Receiver.hpp"

Receiver::Receiver(dvec3 x, float size) : x(x), size(size) {}

void Receiver::receive(double time, BandEnergies &energies,
                       unsigned int emitter_id) {

  size_t bin = static_cast<size_t>(time / bin_width);
  if (bin >= histograms[emitter_id].size()) {
    histograms[emitter_id].resize(bin + 1, BandEnergies{});
  }
  // adds particle's energies to appropriate band at arrival time
  for (int i = 0; i < kNumBands; i++) {
    histograms[emitter_id][bin][i] += energies[i];
  }
}
