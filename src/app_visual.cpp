#include "Scene.hpp"
#include "SimConfig.hpp"
#include "Simulation.hpp"
#include <algorithm>
#include <cmath>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

// Front-end driver: owns the window and the render loop. All physics is
// delegated to Simulation::step(); this file is purely "build a sim, advance it
// one frame, draw it".
int main() {
  const unsigned int POINT_RADIUS = 8;

  SimConfig cfg;
  Simulation sim(make_diamond_scene(20), cfg);

  GLFWwindow *window;
  if (!glfwInit())
    return -1;

  window = glfwCreateWindow(800, 800, "Particle Simulation", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-10.0, 110.0, -10.0, 110.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (!glfwWindowShouldClose(window)) {
    // Advance the physics by one frame.
    sim.step(cfg.dt);

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
      float t = static_cast<float>(std::log10(energy) /
                                   -6); // rescale to 0 for full and 1 at threshold
      t = std::clamp(t, 0.0f, 1.0f);
      glColor3f(1 - t, 1 - t, 1 - 0.8 * t);
      glVertex2f(p.x[0], p.x[1]);
    }
    glEnd();

    // Draw receivers
    glBegin(GL_POINTS);
    for (const Receiver &r : sim.scene.receivers) {
      glPointSize(r.size * 2);
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex2f(r.x[0], r.x[1]);
    }
    glEnd();

    // Draw emitters
    glBegin(GL_POINTS);
    for (const Emitter &e : sim.scene.emitters) {
      glPointSize(2);
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
