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

Scene make_room(float width, float length, float height, Material &material) {
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

Scene make_cathedral(Material &material) {
  Scene scene;

  const double nave_half = 7.0; // nave half-width (arcade at x = +-7)
  const double aisle_w = 6.0;   // side aisle width
  const double aisle_z = 10.0;  // aisle ceiling = top of the nave arcade
  const double eaves_z = 22.0;  // wall head / springing of the nave roof
  const double ridge_z = 30.0;  // ridge of both gables
  const double y_west = -45.0;  // west front
  const double y_tr0 = 8.0;     // transept, west face
  const double y_tr1 = 22.0;    // transept, east face
  const double y_apse = 34.0;   // chancel arch, where the apse begins
  const double arm_x = 25.0;    // transept arms end at x = +-arm_x
  const double apse_r = 7.0;    // apse radius, centred on (0, y_apse)
  const int apse_facets = 5;

  const double rise = ridge_z - eaves_z;
  const double tr_half = (y_tr1 - y_tr0) / 2; // transept half-depth
  const double tr_mid = (y_tr0 + y_tr1) / 2;
  const double aisle_mid = nave_half + aisle_w / 2; // aisle centre-line |x|

  const double nave_slant = std::hypot(nave_half, rise);
  const double tr_slant = std::hypot(tr_half, rise);

  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, (y_west + y_apse + apse_r) / 2, 0}, 2 * arm_x,
            (y_apse + apse_r) - y_west, material));

  const double nave_len = y_apse - y_west;
  scene.planes.emplace_back(Plane({0, 1, 0}, {0, y_west, ridge_z / 2},
                                  2 * nave_half, ridge_z, material));

  const double cler_len = y_tr0 - y_west;
  const double cler_mid = (y_west + y_tr0) / 2;
  scene.planes.emplace_back(
      Plane({-1, 0, 0}, {nave_half, cler_mid, (aisle_z + eaves_z) / 2},
            cler_len, eaves_z - aisle_z, material));
  scene.planes.emplace_back(
      Plane({1, 0, 0}, {-nave_half, cler_mid, (aisle_z + eaves_z) / 2},
            cler_len, eaves_z - aisle_z, material));

  const double choir_len = y_apse - y_tr1;
  const double choir_mid = (y_tr1 + y_apse) / 2;
  scene.planes.emplace_back(Plane({-1, 0, 0},
                                  {nave_half, choir_mid, eaves_z / 2},
                                  choir_len, eaves_z, material));
  scene.planes.emplace_back(Plane({1, 0, 0},
                                  {-nave_half, choir_mid, eaves_z / 2},
                                  choir_len, eaves_z, material));

  const double nave_mid = (y_west + y_apse) / 2;
  scene.planes.emplace_back(
      Plane({-rise, 0, -nave_half},
            {nave_half / 2, nave_mid, (eaves_z + ridge_z) / 2}, nave_len,
            nave_slant, material));
  scene.planes.emplace_back(
      Plane({rise, 0, -nave_half},
            {-nave_half / 2, nave_mid, (eaves_z + ridge_z) / 2}, nave_len,
            nave_slant, material));

  scene.planes.emplace_back(Plane({0, -1, 0},
                                  {0, y_apse, (eaves_z + ridge_z) / 2},
                                  2 * nave_half, ridge_z - eaves_z, material));
  scene.planes.emplace_back(Plane({0, 0, -1}, {0, y_apse + apse_r / 2, eaves_z},
                                  2 * nave_half, apse_r, material));
  const double facet_arc = M_PI / apse_facets;
  const double facet_w = 2 * apse_r * std::sin(facet_arc / 2);
  const double facet_r = apse_r * std::cos(facet_arc / 2); // centre-to-facet
  for (int i = 0; i < apse_facets; ++i) {
    const double theta = facet_arc * (i + 0.5);
    const dvec3 outward{std::cos(theta), std::sin(theta), 0};
    const dvec3 centre{facet_r * outward[0], y_apse + facet_r * outward[1],
                       eaves_z / 2};
    scene.planes.emplace_back(
        Plane(-outward, centre, facet_w, eaves_z, material));
  }

  for (double side : {1.0, -1.0}) {
    const double x_outer = side * (nave_half + aisle_w);
    const double x_mid = side * aisle_mid;
    scene.planes.emplace_back(Plane({-side, 0, 0},
                                    {x_outer, cler_mid, aisle_z / 2}, cler_len,
                                    aisle_z, material));
    scene.planes.emplace_back(Plane({0, 0, -1}, {x_mid, cler_mid, aisle_z},
                                    aisle_w, cler_len, material));
    scene.planes.emplace_back(Plane({0, 1, 0}, {x_mid, y_west, aisle_z / 2},
                                    aisle_w, aisle_z, material));
    scene.planes.emplace_back(Plane({0, -1, 0}, {x_mid, y_tr0, aisle_z / 2},
                                    aisle_w, aisle_z, material));
  }

  const double arm_len = arm_x - nave_half;
  for (double side : {1.0, -1.0}) {
    const double x_mid = side * (nave_half + arm_len / 2);
    scene.planes.emplace_back(Plane({-side, 0, 0},
                                    {side * arm_x, tr_mid, ridge_z / 2},
                                    2 * tr_half, ridge_z, material));
    scene.planes.emplace_back(
        Plane({0, 1, 0}, {side * aisle_mid, y_tr0, (aisle_z + eaves_z) / 2},
              aisle_w, eaves_z - aisle_z, material));
    const double beyond = arm_len - aisle_w; // arm west face beyond the aisle
    scene.planes.emplace_back(
        Plane({0, 1, 0}, {side * (arm_x - beyond / 2), y_tr0, eaves_z / 2},
              beyond, eaves_z, material));
    scene.planes.emplace_back(Plane({0, -1, 0}, {x_mid, y_tr1, eaves_z / 2},
                                    arm_len, eaves_z, material));
    scene.planes.emplace_back(
        Plane({0, rise, -tr_half},
              {x_mid, y_tr0 + tr_half / 2, (eaves_z + ridge_z) / 2}, arm_len,
              tr_slant, material));
    scene.planes.emplace_back(
        Plane({0, -rise, -tr_half},
              {x_mid, y_tr1 - tr_half / 2, (eaves_z + ridge_z) / 2}, arm_len,
              tr_slant, material));
    // Gable end facing the crossing.
    scene.planes.emplace_back(
        Plane({side, 0, 0}, {side * nave_half, tr_mid, (eaves_z + ridge_z) / 2},
              2 * tr_half, ridge_z - eaves_z, material));
  }

  scene.emitters.emplace_back(Emitter{{0.0, y_tr0 - 3.0, 1.7}});
  scene.receivers.emplace_back(Receiver{{1.5, y_tr0 - 13.0, 1.7}, 0.5});

  return scene;
}

Scene make_common_room() {
  Scene scene;

  const double w = 5.0; // x, wall to wall
  const double d = 8.0; // y, wall to wall
  const double h = 2.5; // ceiling height
  const double hw = w / 2;
  const double hd = d / 2;

  // Stud walls, not masonry — see mPlasterboard.
  const Material &plaster = materials::mPlasterboard;
  const Material &carpet = materials::mCarpet;
  const Material &glass = materials::mGlass;
  const Material &wood = materials::mSolidWood;
  const Material &upholstery = materials::mAbsorber;

  // --- Shell --------------------------------------------------------------
  scene.planes.emplace_back(Plane({0, 0, 1}, {0, 0, 0}, w, d, carpet));
  scene.planes.emplace_back(Plane({0, 0, -1}, {0, 0, h}, w, d, wood));
  scene.planes.emplace_back(Plane({0, 1, 0}, {0, -hd, h / 2}, w, h, wood));
  scene.planes.emplace_back(Plane({0, -1, 0}, {0, hd, h / 2}, w, h, wood));

  // --- Window wall (x = +hw), tiled around the glazing ---------------------
  // Tiled rather than overlaid: a patch laid on top of a full wall shares the
  // wall's plane, and which of the two the intersection loop reaches first at
  // an identical t is down to rounding.
  const double win_w = 2.4, win_sill = 0.9, win_head = 2.1;
  const double win_h = win_head - win_sill;
  scene.planes.emplace_back(
      Plane({-1, 0, 0}, {hw, 0, win_sill / 2}, d, win_sill, plaster));
  scene.planes.emplace_back(
      Plane({-1, 0, 0}, {hw, 0, (win_head + h) / 2}, d, h - win_head, plaster));
  scene.planes.emplace_back(Plane(
      {-1, 0, 0}, {hw, 0, (win_sill + win_head) / 2}, win_w, win_h, glass));
  // Curtains, drawn back to either side of the glazing. Hung flat against the
  // reveal instead of floating in front of it, so there is no thin cavity for
  // particles to get trapped in. On a wall this long the curtains no longer
  // reach the corners, so the rest of the window band is plain plaster.
  const double curtain_w = 0.7;
  const double curtain_y = (win_w + curtain_w) / 2;
  const double win_infill_w = (d - win_w) / 2 - curtain_w;
  const double win_infill_y = win_w / 2 + curtain_w + win_infill_w / 2;
  for (double side : {1.0, -1.0}) {
    scene.planes.emplace_back(
        Plane({-1, 0, 0}, {hw, side * curtain_y, (win_sill + win_head) / 2},
              curtain_w, win_h, carpet));
    scene.planes.emplace_back(
        Plane({-1, 0, 0}, {hw, side * win_infill_y, (win_sill + win_head) / 2},
              win_infill_w, win_h, plaster));
  }

  // --- Door wall (x = -hw), tiled around the door -------------------------
  const double door_w = 0.9, door_h = 2.0;
  const double jamb_w = (d - door_w) / 2;
  const double jamb_y = (door_w + jamb_w) / 2;
  scene.planes.emplace_back(
      Plane({1, 0, 0}, {-hw, 0, door_h / 2}, door_w, door_h, wood));
  scene.planes.emplace_back(Plane({1, 0, 0}, {-hw, 0, (door_h + h) / 2}, door_w,
                                  h - door_h, plaster));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({1, 0, 0}, {-hw, side * jamb_y, h / 2}, jamb_w, h, plaster));

  // --- Sofa: against the y = -hd wall, closed by that wall and the floor ---
  const double sofa_w = 2.0, sofa_d = 0.9, sofa_h = 0.8;
  const double sofa_front = -hd + sofa_d;
  scene.planes.emplace_back(Plane({0, 0, 1}, {0, -hd + sofa_d / 2, sofa_h},
                                  sofa_w, sofa_d, upholstery));
  scene.planes.emplace_back(Plane({0, 1, 0}, {0, sofa_front, sofa_h / 2},
                                  sofa_w, sofa_h, upholstery));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({side, 0, 0}, {side * sofa_w / 2, -hd + sofa_d / 2, sofa_h / 2},
              sofa_d, sofa_h, carpet));

  // --- Bookcase: against the y = +hd wall ---------------------------------
  const double bc_w = 1.6, bc_d = 0.35, bc_h = 1.8;
  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, hd - bc_d / 2, bc_h}, bc_w, bc_d, wood));
  scene.planes.emplace_back(
      Plane({0, -1, 0}, {0, hd - bc_d, bc_h / 2}, bc_w, bc_h, wood));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(Plane({side, 0, 0},
                                    {side * bc_w / 2, hd - bc_d / 2, bc_h / 2},
                                    bc_d, bc_h, wood));

  // --- Coffee table: free-standing, closed by the floor -------------------
  const double t_w = 1.1, t_d = 0.6, t_h = 0.45;
  const dvec3 t_c{0, -2.1, 0}; // footprint centre, ~0.7 m off the sofa front
  scene.planes.emplace_back(
      Plane({0, 0, 1}, {t_c[0], t_c[1], t_h}, t_w, t_d, wood));
  for (double side : {1.0, -1.0}) {
    scene.planes.emplace_back(Plane({side, 0, 0},
                                    {t_c[0] + side * t_w / 2, t_c[1], t_h / 2},
                                    t_d, t_h, wood));
    scene.planes.emplace_back(Plane({0, side, 0},
                                    {t_c[0], t_c[1] + side * t_d / 2, t_h / 2},
                                    t_w, t_h, wood));
  }

  // Someone talking from the far half of the room, heard by a seated listener
  // in front of the sofa about 4.3 m away.
  scene.emitters.emplace_back(Emitter{{-1.5, 1.0, 1.6}});
  scene.receivers.emplace_back(Receiver{{0.4, -2.8, 1.1}, 0.1});

  return scene;
}

// Two rooms of very different reverberance sharing a doorway: a small treated
// room (carpet, acoustic-tile ceiling, panelled walls, ~0.2 s) opening into a
// large hard-surfaced hall (~7 s, 45x the volume). Weak coupling — the door is
// ~2 m^2 against ~40 m^2 of absorption in the small room — which is exactly
// the condition for a two-slope decay: the small room empties at its own rate,
// then the hall, still full of energy, feeds back through the door and the
// tail flattens onto the hall's much slower slope. No single RT60 describes
// it, and neither Sabine nor Eyring predicts it.
//
// receivers[0] sits in the small room, where the knee shows; receivers[1] is
// in the hall for comparison. app_offline builds its RIR from receivers[0].
//
// Plan (x across, y through the doorway, z up):
//
//        y = +14  +---------------------------+
//                 |                           |
//                 |   hall  20 x 14 x 9       |   concrete / plaster
//        y =   0  +-----------+ door +--------+   partition, |x| <= 10
//        y = -0.15      +-----+ 1.0 +-----+       partition, |x| <= 2.5
//                       | small room 5x4x2.8|
//        y =  -4        +-------------------+
//
// The partition is two leaves 0.15 m apart rather than one shared wall: a
// single wall would need to face into both rooms at once, and two coplanar
// planes with opposite normals leak, because Particle::move picks whichever it
// finds first at an equal t and nudges the particle to that plane's side. The
// gap costs three extra planes to line the door reveal and is what a real
// partition looks like anyway.
Scene make_coupled_rooms() {
  Scene scene;

  const Material &carpet = materials::mCarpet;
  const Material &tile = materials::mAbsorber;
  const Material &panel = materials::mSolidWood;
  const Material &concrete = materials::mConcrete;
  const Material &plaster = materials::mPlaster;

  const double sw = 5.0, sd = 4.0, sh = 2.8;   // small room
  const double hw = 20.0, hd = 14.0, hh = 9.0; // hall
  const double door_w = 1.0, door_h = 2.1;
  const double leaf = 0.15; // partition thickness

  // --- Small room, y in [-leaf - sd, -leaf] -------------------------------
  const double s_back = -leaf - sd;
  const double s_mid = (s_back - leaf) / 2;
  scene.planes.emplace_back(Plane({0, 0, 1}, {0, s_mid, 0}, sw, sd, carpet));
  scene.planes.emplace_back(Plane({0, 0, -1}, {0, s_mid, sh}, sw, sd, tile));
  scene.planes.emplace_back(
      Plane({0, 1, 0}, {0, s_back, sh / 2}, sw, sh, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({-side, 0, 0}, {side * sw / 2, s_mid, sh / 2}, sd, sh, panel));

  // Small room's leaf of the partition, tiled around the doorway.
  const double s_pier = (sw - door_w) / 2; // wall each side of the door
  scene.planes.emplace_back(
      Plane({0, -1, 0}, {0, -leaf, (door_h + sh) / 2}, sw, sh - door_h, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({0, -1, 0}, {side * (door_w + s_pier) / 2, -leaf, door_h / 2},
              s_pier, door_h, panel));

  // --- Hall, y in [0, hd] -------------------------------------------------
  scene.planes.emplace_back(Plane({0, 0, 1}, {0, hd / 2, 0}, hw, hd, concrete));
  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, hd / 2, hh}, hw, hd, plaster));
  scene.planes.emplace_back(
      Plane({0, -1, 0}, {0, hd, hh / 2}, hw, hh, plaster));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(Plane(
        {-side, 0, 0}, {side * hw / 2, hd / 2, hh / 2}, hd, hh, concrete));

  // Hall's leaf of the partition, tiled around the same doorway.
  const double h_pier = (hw - door_w) / 2;
  scene.planes.emplace_back(
      Plane({0, 1, 0}, {0, 0, (door_h + hh) / 2}, hw, hh - door_h, concrete));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(
        Plane({0, 1, 0}, {side * (door_w + h_pier) / 2, 0, door_h / 2}, h_pier,
              door_h, concrete));

  // --- Door reveal: seals the passage off from the cavity between leaves ---
  scene.planes.emplace_back(
      Plane({0, 0, -1}, {0, -leaf / 2, door_h}, door_w, leaf, panel));
  for (double side : {1.0, -1.0})
    scene.planes.emplace_back(Plane({-side, 0, 0},
                                    {side * door_w / 2, -leaf / 2, door_h / 2},
                                    leaf, door_h, panel));
  // Threshold: the small room's floor already spans up to y = -leaf, so the
  // strip under the passage needs its own slab.
  scene.planes.emplace_back(
      Plane({0, 0, 1}, {0, -leaf / 2, 0}, door_w, leaf, carpet));

  // Source and both receivers off the doorway axis, so no path is privileged
  // by symmetry. Equal receiver radii keep the two levels comparable; 0.25 m
  // is a compromise between capture rate in the hall and time smearing in the
  // small room (~0.7 ms).
  scene.emitters.emplace_back(Emitter{{-1.2, s_mid - 0.5, 1.6}});
  scene.receivers.emplace_back(Receiver{{1.0, s_mid + 0.5, 1.2}, 0.25});
  scene.receivers.emplace_back(Receiver{{3.0, 6.0, 1.6}, 0.25});

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
