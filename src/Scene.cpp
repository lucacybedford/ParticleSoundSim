#include "Scene.hpp"
#include "Materials.hpp"
#include "Receiver.hpp"
#include <cmath>

Scene make_room(float width, float height, Material &material) {
  Scene scene;

  const dvec2 centre{0, 0};

  dvec2 a{centre[0] - (width / 2), centre[1]};
  dvec2 b{centre[0], centre[1] - (height / 2)};
  dvec2 c{centre[0] + (width / 2), centre[1]};
  dvec2 d{centre[0], centre[1] + (height / 2)};

  scene.planes.emplace_back(Plane({1, 0}, a, height, material));
  scene.planes.emplace_back(Plane({0, 1}, b, width, material));
  scene.planes.emplace_back(Plane({-1, 0}, c, height, material));
  scene.planes.emplace_back(Plane({0, -1}, d, width, material));

  scene.emitters.emplace_back(centre + dvec2{0, -3}, 1 * M_PI / 4,
                              3 * M_PI / 4);
  scene.receivers.emplace_back(0, 3, 0.3);

  return scene;
}

Scene make_L_room(Material &mat) {
  Scene scene;

  dvec2 a{0, 0};
  dvec2 b{1.5, 3};
  dvec2 c{-1.5, 6};
  dvec2 d{-4.5, 4.5};
  dvec2 e{-3, 3};
  dvec2 f{-1.5, 1.5};

  scene.planes.emplace_back(Plane{{0, 1}, a, 3, mat});
  scene.planes.emplace_back(Plane{{-1, 0}, b, 6, mat});
  scene.planes.emplace_back(Plane{{0, -1}, c, 6, mat});
  scene.planes.emplace_back(Plane{{1, 0}, d, 3, mat});
  scene.planes.emplace_back(Plane{{0, 1}, e, 3, mat});
  scene.planes.emplace_back(Plane{{1, 0}, f, 3, mat});

  scene.emitters.emplace_back(Emitter{{-3, 4.5}});

  scene.receivers.emplace_back(Receiver{0.0, 1.5, 0.3});

  return scene;
}

Scene make_10x5_absorber_room() {
  Scene scene;

  const dvec2 centre{0, 0};

  const double height = 10;
  const double width = 5;

  dvec2 a{centre[0] - (width / 2), centre[1]};
  dvec2 b{centre[0], centre[1] - (height / 2)};
  dvec2 c{centre[0] + (width / 2), centre[1]};
  dvec2 d{centre[0], centre[1] + (height / 2)};

  scene.planes.emplace_back(Plane({1, 0}, a, height, materials::mAbsorber));
  scene.planes.emplace_back(Plane({0, 1}, b, width, materials::mAbsorber));
  scene.planes.emplace_back(Plane({-1, 0}, c, height, materials::mAbsorber));
  scene.planes.emplace_back(Plane({0, -1}, d, width, materials::mAbsorber));

  scene.emitters.emplace_back(centre + dvec2{0, -3}, 1 * M_PI / 4,
                              3 * M_PI / 4);
  scene.receivers.emplace_back(0, 3, 0.3);

  return scene;
}

Scene make_10x5_wood_room() {
  Scene scene;

  const dvec2 centre{0, 0};

  const double height = 10;
  const double width = 5;

  dvec2 a{centre[0] - (width / 2), centre[1]};
  dvec2 b{centre[0], centre[1] - (height / 2)};
  dvec2 c{centre[0] + (width / 2), centre[1]};
  dvec2 d{centre[0], centre[1] + (height / 2)};

  scene.planes.emplace_back(Plane({1, 0}, a, height, materials::mSolidWood));
  scene.planes.emplace_back(Plane({0, 1}, b, width, materials::mSolidWood));
  scene.planes.emplace_back(Plane({-1, 0}, c, height, materials::mSolidWood));
  scene.planes.emplace_back(Plane({0, -1}, d, width, materials::mSolidWood));

  scene.emitters.emplace_back(centre + dvec2{0, -3}, 1 * M_PI / 4,
                              3 * M_PI / 4);
  scene.receivers.emplace_back(0, 3, 0.3);

  return scene;
}

Scene make_10x5_concrete_room() {
  Scene scene;

  const dvec2 centre{0, 0};

  const double height = 10;
  const double width = 5;

  dvec2 a{centre[0] - (width / 2), centre[1]};
  dvec2 b{centre[0], centre[1] - (height / 2)};
  dvec2 c{centre[0] + (width / 2), centre[1]};
  dvec2 d{centre[0], centre[1] + (height / 2)};

  scene.planes.emplace_back(Plane({1, 0}, a, height, materials::mConcrete));
  scene.planes.emplace_back(Plane({0, 1}, b, width, materials::mConcrete));
  scene.planes.emplace_back(Plane({-1, 0}, c, height, materials::mConcrete));
  scene.planes.emplace_back(Plane({0, -1}, d, width, materials::mConcrete));

  scene.emitters.emplace_back(centre + dvec2{0, -3}, 1 * M_PI / 4,
                              3 * M_PI / 4);
  scene.receivers.emplace_back(0, 3, 0.3);

  return scene;
}

Scene make_30x10_concrete_room() {
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

  scene.emitters.emplace_back(centre + dvec2{0, -3}, 1 * M_PI / 4,
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
