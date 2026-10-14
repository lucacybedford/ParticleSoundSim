#include "Plane.hpp"
#include <cmath>
#include <cstddef>
#include <glm/geometric.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

// any unit vector perpendicular to n
static dvec3 tangent_for(const dvec3 &n) {
  if (std::abs(n[2]) > 0.999)
    return {1, 0, 0};
  return glm::normalize(dvec3{-n[1], n[0], 0});
}

static std::vector<dvec3> rect_corners(dvec3 normal, dvec3 point, double length,
                                       double height) {
  const dvec3 h_tan = tangent_for(normal);
  const dvec3 v_tan = glm::cross(normal, h_tan);
  const dvec3 half_l = h_tan * (length / 2);
  const dvec3 half_h = v_tan * (height / 2);
  return {point - half_l - half_h, point + half_l - half_h,
          point + half_l + half_h, point - half_l + half_h};
}

Plane::Plane(std::vector<dvec3> vertices, const Material &material)
    : corners(std::move(vertices)), material(material) {
  const std::size_t k = corners.size();
  if (k < 3)
    throw std::invalid_argument("Plane: polygon needs at least 3 vertices");

  p = dvec3{0};
  for (const dvec3 &v : corners)
    p += v;
  p /= static_cast<double>(k);

  // Newell's method for calculating normal of polygon
  dvec3 sum{0};
  for (std::size_t i = 0; i < k; ++i)
    sum += glm::cross(corners[i] - p, corners[(i + 1) % k] - p);
  if (glm::length(sum) < 1e-9)
    throw std::invalid_argument("Plane: degenerate polygon (zero area)");
  n = glm::normalize(sum);

  // get tangents
  h_tangent = tangent_for(n);
  v_tangent = glm::cross(n, h_tangent);

  edge_normals.reserve(k);
  for (std::size_t i = 0; i < k; ++i) {
    const dvec3 edge = corners[(i + 1) % k] - corners[i];
    edge_normals.push_back(glm::normalize(glm::cross(n, edge)));
  }
}

Plane::Plane(dvec3 normal, dvec3 point, double length, double height,
             const Material &material)
    : Plane(rect_corners(glm::normalize(normal), point, length, height),
            material) {}
