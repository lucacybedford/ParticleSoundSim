#pragma once
#include <array>
#include <glm/glm.hpp>

using glm::dvec2;

struct Plane {
  dvec2 n;
  dvec2 p;
  dvec2 tangent;
  dvec2 end_a;
  dvec2 end_b;
  double l;
  std::array<double, 8> absorption{0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

  Plane(dvec2 normal, dvec2 point, double length);
};
