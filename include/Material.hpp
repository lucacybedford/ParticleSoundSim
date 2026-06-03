#pragma once
#include <array>
#include <string>

struct Material {
  std::string name;
  std::array<double, 8> absorption;
};
