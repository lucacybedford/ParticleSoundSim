#include "Receiver.hpp"

Receiver::Receiver(double x, double y, float size) : x(x, y), size(size) {}

void Receiver::receive(double time, std::array<double, 8> &energies) {
  size_t bin = static_cast<size_t>(time / bin_width);
  if (bin >= histogram.size()) {
    histogram.resize(bin + 1, {0, 0, 0, 0, 0, 0, 0, 0});
  }
  // adds particle's energies to appropriate band at arrival time
  for (size_t i = 0; i < 8; i++) {
    histogram[bin][i] += energies[i];
  }
}
