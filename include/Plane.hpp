#pragma once
#include "Material.hpp"
#include <glm/glm.hpp>
#include <vector>

using glm::dvec3;

struct Plane {
  dvec3 n; // normal to plane
  dvec3 p; // centroid of polygon
  dvec3 h_tangent, v_tangent;
  std::vector<dvec3> corners;
  std::vector<dvec3> edge_normals;
  Material material;

  Plane(std::vector<dvec3> vertices, const Material &material); // new, primary
  Plane(dvec3 normal, dvec3 point, double length, double height,
        const Material &material); // builds 4 corners
  bool contains(const dvec3 &x) const;
};
