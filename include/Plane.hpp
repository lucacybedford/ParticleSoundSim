#pragma once
#include "Material.hpp"
#include <array>
#include <glm/glm.hpp>

using glm::dvec3;

struct Plane {
  dvec3 n;
  dvec3 p;
  double l;
  double h;
  dvec3 h_tangent;
  dvec3 v_tangent;
  std::array<dvec3, 4> corners; // counter-clockwise when viewed from the front

  BandEnergies absorption;

  Plane(dvec3 normal, dvec3 point, double length, double height,
        const Material &material);
};
