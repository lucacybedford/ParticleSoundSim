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

Scene make_diamond_scene(double room_radius = 2.4);
Scene make_big_box();
