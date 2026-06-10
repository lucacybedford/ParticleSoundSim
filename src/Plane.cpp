#include "Plane.hpp"
#include <cmath>
#include <glm/geometric.hpp>

Plane::Plane(dvec3 normal, dvec3 point, double length, double height,
             const Material &material)
    : n(glm::normalize(normal)), p(point), l(length), h(height),
      absorption(material.absorption) {
  if (std::abs(n[2]) > 0.999) {
    h_tangent = {1, 0, 0};
  } else {
    h_tangent = glm::normalize(dvec3{-n[1], n[0], 0});
  }
  v_tangent = glm::cross(n, h_tangent);
  h_end_a = p - h_tangent * (l / 2);
  h_end_b = p + h_tangent * (l / 2);
  v_end_a = p - v_tangent * (h / 2);
  v_end_b = p + v_tangent * (h / 2);
}
