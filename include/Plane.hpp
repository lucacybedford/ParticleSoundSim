#pragma once
#include "Material.hpp"
#include <glm/glm.hpp>

using glm::dvec3;

struct Plane {
  dvec3 n;
  dvec3 p;
  double l;
  double h;
  dvec3 h_tangent;
  dvec3 v_tangent;
  dvec3 h_end_a;
  dvec3 h_end_b;
  dvec3 v_end_a;
  dvec3 v_end_b;
  BandEnergies absorption;

  Plane(dvec3 normal, dvec3 point, double length, double height,
        const Material &material);
};
