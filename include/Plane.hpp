#pragma once
#include "Material.hpp"
#include <array>
#include <glm/glm.hpp>

using glm::dvec2;

struct Plane {
  dvec2 n;
  dvec2 p;
  double l;
  dvec2 tangent;
  dvec2 end_a;
  dvec2 end_b;
  std::array<double, 8> absorption;

  Plane(dvec2 normal, dvec2 point, double length, const Material &material);
};
