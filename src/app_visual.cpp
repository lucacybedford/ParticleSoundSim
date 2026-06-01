#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <algorithm>
#include <cmath>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

int main() {
  const unsigned int POINT_RADIUS = 8;

  SimConfig cfg;
  cfg.fidelity = SimConfig::Fidelity::Realtime;
  cfg.playback_speed = 0.05; // 20x slow-motion (real sound speed, slow display)

  Atmosphere air;
  Simulation sim(make_big_box(), cfg, air);

  GLFWwindow *window;
  if (!glfwInit())
    return -1;

  window = glfwCreateWindow(800, 800, "Particle Simulation", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  // fit camera to room dimensions
  double minx = 1e30, miny = 1e30, maxx = -1e30, maxy = -1e30;
  for (const Plane &pl : sim.scene.planes) {
    for (const dvec2 &pt : {pl.end_a, pl.end_b}) {
      minx = std::min(minx, pt[0]);
      maxx = std::max(maxx, pt[0]);
      miny = std::min(miny, pt[1]);
      maxy = std::max(maxy, pt[1]);
    }
  }
  double margin = 0.1 * std::max(maxx - minx, maxy - miny);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(minx - margin, maxx + margin, miny - margin, maxy + margin, -1.0,
          1.0);
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (!glfwWindowShouldClose(window)) {
    sim.step(cfg.dt * cfg.playback_speed);

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw room
    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(1, 1, 1);
    for (const Plane &plane : sim.scene.planes) {
      glVertex2f(plane.end_a[0], plane.end_a[1]);
      glVertex2f(plane.end_b[0], plane.end_b[1]);
    }
    glEnd();

    // Draw particles
    glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Particle &p : sim.particles) {
      double energy = p.check_energy();
      float t = static_cast<float>(
          std::log10(energy) / -6); // rescale to 0 for full and 1 at threshold
      t = std::clamp(t, 0.0f, 1.0f);
      glColor3f(1 - t, 1 - t, 1 - 0.8 * t);
      glVertex2f(p.x[0], p.x[1]);
    }
    glEnd();

    // Draw receivers
    glPointSize(14);
    glBegin(GL_POINTS);
    for (const Receiver &r : sim.scene.receivers) {
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex2f(r.x[0], r.x[1]);
    }
    glEnd();

    // Draw emitters
    glPointSize(10);
    glBegin(GL_POINTS);
    for (const Emitter &e : sim.scene.emitters) {
      glColor3f(0.7f, 1.0f, 0.0f);
      glVertex2f(e.x[0], e.x[1]);
    }
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
