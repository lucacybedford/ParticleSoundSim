#include "Scene.hpp"
#include <cmath>

Scene make_diamond_scene(double size) {
  Scene scene;

  // Four corners of the square, then four planes facing inward (this is the
  // old diamondPlanes()). The 2.9 factor is the original display length.
  dvec2 a{50 - size, 50 - size};
  dvec2 b{50 + size, 50 - size};
  dvec2 c{50 + size, 50 + size};
  dvec2 d{50 - size, 50 + size};

  scene.planes.emplace_back(Plane({1, 1}, a, size * 2.9));
  scene.planes.emplace_back(Plane({-1, 1}, b, size * 2.9));
  scene.planes.emplace_back(Plane({-1, -1}, c, size * 2.9));
  scene.planes.emplace_back(Plane({1, -1}, d, size * 2.9));

  // Two emitters firing into opposite half-planes (from the old main()).
  scene.emitters.emplace_back(dvec2{50, 40}, 5 * M_PI / 4, 7 * M_PI / 4);
  scene.emitters.emplace_back(dvec2{50, 60}, 1 * M_PI / 4, 3 * M_PI / 4);

  // One receiver.
  scene.receivers.emplace_back(60, 60, 2);

  return scene;
}
