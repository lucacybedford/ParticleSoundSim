#include "Receiver.hpp"

Receiver::Receiver(dvec3 x, float size) : x(x), size(size) {}

void Receiver::receive(double time, BandEnergies &energies) {
  size_t bin = static_cast<size_t>(time / bin_width);
  if (bin >= histogram.size()) {
    histogram.resize(bin + 1, BandEnergies{});
  }
  // adds particle's energies to appropriate band at arrival time
  for (int i = 0; i < kNumBands; i++) {
    histogram[bin][i] += energies[i];
  }
}
