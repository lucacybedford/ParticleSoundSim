#pragma once
#include "Emitter.hpp"
#include "Plane.hpp"
#include "Receiver.hpp"
#include <vector>

// the Scene holds all room setup info
struct Scene {
  std::vector<Plane> planes;
  std::vector<Emitter> emitters;
  std::vector<Receiver> receivers;
};

Scene make_room(double width, double length, double height,
                const Material &material);
Scene make_cathedral(const Material &material);
Scene make_common_room();
Scene make_coupled_rooms();
Scene make_diamond_scene(double room_radius = 2.4);
Scene make_L_room(const Material &material);
Scene make_standard();
