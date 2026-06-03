#include "Scene.hpp"
#include "Materials.hpp"
#include <cmath>

Scene make_big_box() {
  Scene scene;

  const dvec2 centre{0, 0};

  const double height = 30;
  const double width = 10;

  dvec2 a{centre[0] - (width / 2), centre[1]};
  dvec2 b{centre[0], centre[1] - (height / 2)};
  dvec2 c{centre[0] + (width / 2), centre[1]};
  dvec2 d{centre[0], centre[1] + (height / 2)};

  scene.planes.emplace_back(Plane({1, 0}, a, height, materials::mConcrete));
  scene.planes.emplace_back(Plane({0, 1}, b, width, materials::mConcrete));
  scene.planes.emplace_back(Plane({-1, 0}, c, height, materials::mConcrete));
  scene.planes.emplace_back(Plane({0, -1}, d, width, materials::mConcrete));

  scene.emitters.emplace_back(centre + dvec2{0, -2}, 1 * M_PI / 4,
                              3 * M_PI / 4);
  scene.receivers.emplace_back(0, 3, 0.3);

  return scene;
}

Scene make_diamond_scene(double room_radius) {
  Scene scene;

  const dvec2 centre{2.5, 2.5};

  const double size = room_radius / std::sqrt(2.0);

  dvec2 a = centre + dvec2{-size, -size};
  dvec2 b = centre + dvec2{size, -size};
  dvec2 c = centre + dvec2{size, size};
  dvec2 d = centre + dvec2{-size, size};

  scene.planes.emplace_back(Plane({1, 1}, a, size * 2.9, materials::mPlaster));
  scene.planes.emplace_back(
      Plane({-1, 1}, b, size * 2.9, materials::mSolidWood));
  scene.planes.emplace_back(
      Plane({-1, -1}, c, size * 2.9, materials::mPlaster));
  scene.planes.emplace_back(
      Plane({1, -1}, d, size * 2.9, materials::mAbsorber));

  scene.emitters.emplace_back(centre + dvec2{0, -0.5}, 5 * M_PI / 4,
                              7 * M_PI / 4);
  scene.emitters.emplace_back(centre + dvec2{0, 0.5}, 1 * M_PI / 4,
                              3 * M_PI / 4);

  scene.receivers.emplace_back(3.3, 3.3, 0.2);

  return scene;
}
