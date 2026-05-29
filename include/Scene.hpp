#pragma once
#include "Emitter.hpp"
#include "Plane.hpp"
#include "Receiver.hpp"
#include <vector>

// A Scene is just the static description of a room: its walls, where sound is
// emitted from, and where it is measured. It holds NO particles and NO
// simulation state -- that lives in Simulation. Separating "what the room is"
// from "what is happening in it" is what lets the offline and visual apps share
// the exact same geometry while running it differently.
struct Scene {
  std::vector<Plane> planes;
  std::vector<Emitter> emitters;
  std::vector<Receiver> receivers;
};

// Builds the test scene that main() used to construct inline: a rotated square
// room from four diagonal planes, two emitters, one receiver.
Scene make_diamond_scene(double size);
