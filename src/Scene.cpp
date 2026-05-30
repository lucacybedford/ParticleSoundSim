#include "Scene.hpp"
#include <cmath>

// All coordinates are in METRES (1 unit = 1 m).
//
// `room_radius` is the perpendicular distance from the centre to each diagonal
// wall. The diamond's flats end up 2*room_radius apart, so the default 2.4 m
// gives a ~4.8 m room.
Scene make_diamond_scene(double room_radius) {
  Scene scene;

  const dvec2 centre{2.5, 2.5};

  // The four diagonal walls have normals {±1,±1}. A wall's perpendicular
  // distance from the centre is size*sqrt(2), so to place it at room_radius we
  // offset each corner anchor by size along both axes.
  const double size = room_radius / std::sqrt(2.0);

  dvec2 a = centre + dvec2{-size, -size};
  dvec2 b = centre + dvec2{size, -size};
  dvec2 c = centre + dvec2{size, size};
  dvec2 d = centre + dvec2{-size, size};

  // 2.9*size is a display length slightly over the diagonal so the walls
  // overlap at the corners and fully enclose the room.
  scene.planes.emplace_back(Plane({1, 1}, a, size * 2.9));
  scene.planes.emplace_back(Plane({-1, 1}, b, size * 2.9));
  scene.planes.emplace_back(Plane({-1, -1}, c, size * 2.9));
  scene.planes.emplace_back(Plane({1, -1}, d, size * 2.9));

  // Two emitters near the centre, firing into opposite half-planes.
  scene.emitters.emplace_back(centre + dvec2{0, -0.5}, 5 * M_PI / 4,
                              7 * M_PI / 4);
  scene.emitters.emplace_back(centre + dvec2{0, 0.5}, 1 * M_PI / 4,
                              3 * M_PI / 4);

  // One receiver with a 0.2 m capture radius, offset toward a corner.
  scene.receivers.emplace_back(3.3, 3.3, 0.2);

  return scene;
}
