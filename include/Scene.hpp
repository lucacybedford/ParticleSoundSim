#pragma once
#include "Emitter.hpp"
#include "Plane.hpp"
#include "Receiver.hpp"
#include <vector>

// The Scene holds all room setup info
struct Scene {
  std::vector<Plane> planes;
  std::vector<Emitter> emitters;
  std::vector<Receiver> receivers;
};

Scene make_room(float width, float length, float height, Material &material);
Scene make_diamond_scene(double room_radius = 2.4);
Scene make_L_room(Material &material);
Scene make_standard();
