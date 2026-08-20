#include "Scene.hpp"
#include "Emitter.hpp"
#include "Materials.hpp"
#include "Receiver.hpp"
#include <algorithm>
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

Scene make_room(double width, double length, double height,
                const Material &material) {
  Scene scene;

  dvec3 a{-(width / 2), 0, height / 2};
  dvec3 b{0, 0 - (length / 2), height / 2};
  dvec3 c{(width / 2), 0, height / 2};
  dvec3 d{0, (length / 2), height / 2};

  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, 0, 0}, width, length, material));
  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, 0, height}, width, length, material));

  scene.planes.emplace_back(Plane({1, 0, 0}, a, length, height, material));
  scene.planes.emplace_back(Plane({0, 1, 0}, b, width, height, material));
  scene.planes.emplace_back(Plane({-1, 0, 0}, c, length, height, material));
  scene.planes.emplace_back(Plane({0, -1, 0}, d, width, height, material));

  const double src_z = std::min(1.6, 0.25 * height);
  const double rcv_z = std::min(1.2, 0.20 * height);

  scene.emitters.emplace_back(dvec3{0.10 * width, -0.30 * length, src_z});
  scene.receivers.emplace_back(
      Receiver{{-0.06 * width, -0.08 * length, rcv_z}, 0.5});

  return scene;
}

Scene make_coupled_rooms() {
  Scene scene;

  const Material &carpet = materials::mCarpet;
  const Material &tile = materials::mAbsorber;
  const Material &panel = materials::mSolidWood;
  const Material &concrete = materials::mConcrete;
  const Material &plaster = materials::mPlaster;

  const double sw = 5.0, sd = 4.0, sh = 2.8;
  const double hw = 20.0, hd = 14.0, hh = 9.0;
  const double door_w = 1.0, door_h = 2.1;
  const double leaf = 0.15;

  const double s_back = -leaf - sd;
  const double s_mid = (s_back - leaf) / 2;
  scene.planes.emplace_back(Plane({0, 0, 1}, {0, s_mid, 0}, sw, sd, carpet));
  scene.planes.emplace_back(Plane({0, 0, -1}, {0, s_mid, sh}, sw, sd, tile));
  scene.planes.emplace_back(
      Plane({0, 1, 0}, {0, s_back, sh / 2}, sw, sh, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({-side, 0, 0}, {side * sw / 2, s_mid, sh / 2}, sd, sh, panel));

  const double s_pier = (sw - door_w) / 2; // wall each side of the door
  scene.planes.emplace_back(
      Plane({0, -1, 0}, {0, -leaf, (door_h + sh) / 2}, sw, sh - door_h, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({0, -1, 0}, {side * (door_w + s_pier) / 2, -leaf, door_h / 2},
              s_pier, door_h, panel));

  scene.planes.emplace_back(Plane({0, 0, 1}, {0, hd / 2, 0}, hw, hd, concrete));
  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, hd / 2, hh}, hw, hd, plaster));
  scene.planes.emplace_back(
      Plane({0, -1, 0}, {0, hd, hh / 2}, hw, hh, plaster));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(Plane(
        {-side, 0, 0}, {side * hw / 2, hd / 2, hh / 2}, hd, hh, concrete));

  const double h_pier = (hw - door_w) / 2;
  scene.planes.emplace_back(
      Plane({0, 1, 0}, {0, 0, (door_h + hh) / 2}, hw, hh - door_h, concrete));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({0, 1, 0}, {side * (door_w + h_pier) / 2, 0, door_h / 2}, h_pier,
              door_h, concrete));

  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, -leaf / 2, door_h}, door_w, leaf, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(Plane({-side, 0, 0},
                                    {side * door_w / 2, -leaf / 2, door_h / 2},
                                    leaf, door_h, panel));
  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, -leaf / 2, 0}, door_w, leaf, carpet));

  scene.emitters.emplace_back(Emitter{{-1.2, s_mid - 0.5, 1.6}});
  scene.receivers.emplace_back(Receiver{{1.0, s_mid + 0.5, 1.2}, 0.25});
  scene.receivers.emplace_back(Receiver{{3.0, 6.0, 1.6}, 0.25});

  return scene;
}

Scene make_L_room(const Material &mat) {
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
