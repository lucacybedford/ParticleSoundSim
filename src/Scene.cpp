#include "Scene.hpp"
#include <cmath>

Scene make_diamond_scene(double room_radius) {
  Scene scene;

  const dvec2 centre{2.5, 2.5};

  const double size = room_radius / std::sqrt(2.0);

  dvec2 a = centre + dvec2{-size, -size};
  dvec2 b = centre + dvec2{size, -size};
  dvec2 c = centre + dvec2{size, size};
  dvec2 d = centre + dvec2{-size, size};

  scene.planes.emplace_back(Plane({1, 1}, a, size * 2.9));
  scene.planes.emplace_back(Plane({-1, 1}, b, size * 2.9));
  scene.planes.emplace_back(Plane({-1, -1}, c, size * 2.9));
  scene.planes.emplace_back(Plane({1, -1}, d, size * 2.9));

  scene.emitters.emplace_back(centre + dvec2{0, -0.5}, 5 * M_PI / 4,
                              7 * M_PI / 4);
  scene.emitters.emplace_back(centre + dvec2{0, 0.5}, 1 * M_PI / 4,
                              3 * M_PI / 4);

  scene.receivers.emplace_back(3.3, 3.3, 0.2);

  return scene;
}
