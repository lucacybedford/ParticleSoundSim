#include "Plane.hpp"
#include <cmath>
#include <glm/geometric.hpp>

Plane::Plane(dvec3 normal, dvec3 point, double length, double height,
             const Material &material)
    : n(glm::normalize(normal)), p(point), l(length), h(height),
      material(material) {
  if (std::abs(n[2]) > 0.999) {
    h_tangent = {1, 0, 0};
  } else {
    h_tangent = glm::normalize(dvec3{-n[1], n[0], 0});
  }
  v_tangent = glm::cross(n, h_tangent);
  const dvec3 half_l = h_tangent * (l / 2);
  const dvec3 half_h = v_tangent * (h / 2);
  corners = {p - half_l - half_h, p + half_l - half_h, p + half_l + half_h,
             p - half_l + half_h};
}
