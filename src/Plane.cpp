#include "Plane.hpp"
#include <glm/geometric.hpp>

Plane::Plane(dvec2 normal, dvec2 point, double length, const Material &material)
    : n(glm::normalize(normal)), p(point), l(length),
      absorption(material.absorption) {
  tangent = {-n[1], n[0]};
  end_a = p - tangent * (l / 2);
  end_b = p + tangent * (l / 2);
}
