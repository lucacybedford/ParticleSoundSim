#include "Materials.hpp"
#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/trigonometric.hpp"
#include <algorithm>
#include <cmath>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Orbit-distance multiplier (in units of scene radius), adjusted by scrolling.
static double g_zoom = 2.0;

static void scroll_callback(GLFWwindow *, double, double yoffset) {
  g_zoom *= std::pow(0.9, yoffset); // scroll up = zoom in
  g_zoom = std::clamp(g_zoom, 0.2, 8.0);
}

int main() {
  const unsigned int POINT_RADIUS = 8;

  SimConfig cfg;
  cfg.num_particles = 1000;
  cfg.fidelity = SimConfig::Fidelity::Realtime;
  cfg.playback_speed = 0.05; // 20x slow-motion

  Material room_material = materials::mConcrete;

  Atmosphere air;
  Simulation sim(make_room(5, 10, 3, room_material), cfg, air);
  // Simulation sim(make_standard(), cfg, air);

  if (cfg.dt > Receiver::bin_width) {
    printf("dt must be smaller than receiver bin width.");
    return 1;
  }

  GLFWwindow *window;
  if (!glfwInit())
    return -1;

  window = glfwCreateWindow(800, 800, "Particle Simulation", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetScrollCallback(window, scroll_callback);

  dvec3 bb_min{1e30}, bb_max{-1e30};
  for (const Plane &pl : sim.scene.planes) {
    for (const dvec3 &c : pl.corners) {
      bb_min = glm::min(bb_min, c);
      bb_max = glm::max(bb_max, c);
    }
  }
  dvec3 centre = (bb_min + bb_max) / 2.0;
  double radius = glm::length(bb_max - bb_min) / 2;

  glm::dmat4 proj = glm::perspective(glm::radians(45.0), 1.0, 0.1, radius * 10);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixd(glm::value_ptr(proj));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Orbit camera state (spherical coordinates around the scene centre)
  double yaw = 0.5;   // azimuth, radians
  double pitch = 0.5; // elevation, radians
  bool dragging = false;
  double last_x = 0, last_y = 0;

  while (!glfwWindowShouldClose(window)) {
    sim.step(cfg.dt * cfg.playback_speed);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Left-drag to orbit the camera around the scene centre
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      if (dragging) {
        yaw -= (mx - last_x) * 0.01;
        pitch += (my - last_y) * 0.01;
        pitch = std::clamp(pitch, -1.55, 1.55); // avoid flipping over the poles
      }
      dragging = true;
    } else {
      dragging = false;
    }
    last_x = mx;
    last_y = my;

    dvec3 eye =
        centre + g_zoom * radius *
                     dvec3{std::cos(pitch) * std::cos(yaw),
                           std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
    glm::dmat4 view = glm::lookAt(eye, centre, dvec3{0, 0, 1});
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(glm::value_ptr(view));

    // Draw room
    glLineWidth(2);
    glColor3f(1, 1, 1);
    for (const Plane &plane : sim.scene.planes) {
      glBegin(GL_LINE_LOOP);
      for (const dvec3 &c : plane.corners) {
        glVertex3d(c[0], c[1], c[2]);
      }
      glEnd();
    }

    // Draw particles
    glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Particle &p : sim.particles) {
      double energy = p.check_energy();
      float t = static_cast<float>(
          std::log10(energy) / -6); // rescale to 0 for full and 1 at threshold
      t = std::clamp(t, 0.0f, 1.0f);
      glColor3f(1 - t, 1 - t, 1 - 0.8 * t);
      glVertex3d(p.x[0], p.x[1], p.x[2]);
    }
    glEnd();

    // Draw receivers
    glPointSize(14);
    glBegin(GL_POINTS);
    for (const Receiver &r : sim.scene.receivers) {
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3d(r.x[0], r.x[1], r.x[2]);
    }
    glEnd();

    // Draw emitters
    glPointSize(10);
    glBegin(GL_POINTS);
    for (const Emitter &e : sim.scene.emitters) {
      glColor3f(0.7f, 1.0f, 0.0f);
      glVertex3d(e.x[0], e.x[1], e.x[2]);
    }
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
