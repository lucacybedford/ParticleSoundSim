#include "Scene.hpp"
#include "Emitter.hpp"
#include "Materials.hpp"
#include "Receiver.hpp"
#include <cmath>

Scene make_standard() {
  Scene scene;

  double Lx = 3.432;
  double Ly = 5.148;
  double Lz = 4.29;

  Material mat_floor = materials::make(
      "floor", {0.51, 0.51, 0.51, 0.51, 0.51, 0.51, 0.51, 0.51}, 0.3);
  Material mat_wall = materials::make(
      "wall", {0.19, 0.19, 0.19, 0.19, 0.19, 0.19, 0.19, 0.19}, 0.2);

  dvec3 a{0, Ly / 2, Lz / 2};
  dvec3 b{Lx / 2, 0, Lz / 2};
  dvec3 c{Lx, Ly / 2, Lz / 2};
  dvec3 d{Lx / 2, Ly, Lz / 2};

  dvec3 floor{Lx / 2, Ly / 2, 0};
  dvec3 ceiling{Lx / 2, Ly / 2, Lz};

  scene.planes.emplace_back(Plane{{0, 0, 1}, floor, Lx, Ly, mat_floor});
  scene.planes.emplace_back(Plane{{0, 0, -1}, ceiling, Lx, Ly, mat_floor});

  scene.planes.emplace_back(Plane{{1, 0, 0}, a, Ly, Lz, mat_wall});
  scene.planes.emplace_back(Plane({0, 1, 0}, b, Lx, Lz, mat_wall));
  scene.planes.emplace_back(Plane({-1, 0, 0}, c, Ly, Lz, mat_wall));
  scene.planes.emplace_back(Plane({0, -1, 0}, d, Lx, Lz, mat_wall));

  scene.emitters.emplace_back(Emitter{{2.145, 0.429, 2.574}});
  scene.receivers.emplace_back(Receiver{{1.287, 4.29, 1.716}, 0.1});

  return scene;
}

Scene make_room(float width, float length, float height, Material &material) {
  Scene scene;

  dvec3 a{-(width / 2), 0, 1.5};
  dvec3 b{0, 0 - (length / 2), 1.5};
  dvec3 c{(width / 2), 0, 1.5};
  dvec3 d{0, (length / 2), 1.5};

  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, 0, 0}, width, length, material));
  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, 0, height}, width, length, material));

  scene.planes.emplace_back(Plane({1, 0, 0}, a, length, height, material));
  scene.planes.emplace_back(Plane({0, 1, 0}, b, width, height, material));
  scene.planes.emplace_back(Plane({-1, 0, 0}, c, length, height, material));
  scene.planes.emplace_back(Plane({0, -1, 0}, d, width, height, material));

  scene.emitters.emplace_back(dvec3{0, -3, 1.7}, 1 * M_PI / 4, 3 * M_PI / 4,
                              -M_PI / 2, M_PI / 2);
  scene.receivers.emplace_back(Receiver{{0, 3, 1.7}, 0.1});

  return scene;
}

Scene make_L_room(Material &mat) {
  Scene scene;

  dvec3 a{0, 0, 1.5};
  dvec3 b{1.5, 3, 1.5};
  dvec3 c{-1.5, 6, 1.5};
  dvec3 d{-4.5, 4.5, 1.5};
  dvec3 e{-3, 3, 1.5};
  dvec3 f{-1.5, 1.5, 1.5};

  scene.planes.emplace_back(Plane{{0, 1, 0}, a, 3, 3, mat});
  scene.planes.emplace_back(Plane{{-1, 0, 0}, b, 6, 3, mat});
  scene.planes.emplace_back(Plane{{0, -1, 0}, c, 6, 3, mat});
  scene.planes.emplace_back(Plane{{1, 0, 0}, d, 3, 3, mat});
  scene.planes.emplace_back(Plane{{0, 1, 0}, e, 3, 3, mat});
  scene.planes.emplace_back(Plane{{1, 0, 0}, f, 3, 3, mat});

  scene.emitters.emplace_back(Emitter{{-3, 4.5, 1.7}});

  scene.receivers.emplace_back(Receiver{{0.0, 1.5, 1.7}, 0.1});

  return scene;
}

Scene make_diamond_scene(double room_radius) {
  Scene scene;

  const dvec3 centre{2.5, 2.5, 0};

  const double size = room_radius / std::sqrt(2.0);

  dvec3 a = centre + dvec3{-size, -size, 0};
  dvec3 b = centre + dvec3{size, -size, 0};
  dvec3 c = centre + dvec3{size, size, 0};
  dvec3 d = centre + dvec3{-size, size, 0};

  scene.planes.emplace_back(
      Plane({1, 1, 0}, a, size * 2.9, 3, materials::mPlaster));
  scene.planes.emplace_back(
      Plane({-1, 1, 0}, b, size * 2.9, 3, materials::mSolidWood));
  scene.planes.emplace_back(
      Plane({-1, -1, 0}, c, size * 2.9, 3, materials::mPlaster));
  scene.planes.emplace_back(
      Plane({1, -1, 0}, d, size * 2.9, 3, materials::mAbsorber));

  scene.emitters.emplace_back(centre + dvec3{0, -0.5, 1.7}, 5 * M_PI / 4,
                              7 * M_PI / 4, -M_PI / 2, M_PI / 2);
  scene.emitters.emplace_back(centre + dvec3{0, 0.5, 1.7}, 1 * M_PI / 4,
                              3 * M_PI / 4, -M_PI / 2, M_PI / 2);

  scene.receivers.emplace_back(dvec3{3.3, 3.3, 1.7}, 0.1);

  return scene;
}
