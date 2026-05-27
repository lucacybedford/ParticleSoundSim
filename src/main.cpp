#include "Particle.hpp"
#include "Plane.hpp"
#include "Receiver.hpp"
#include <algorithm>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <algorithm>
#include <random>
#include <vector>

std::vector<Plane> diamondPlanes(double size) {
  std::vector<Plane> planes;

  dvec2 a{50 - size, 50 - size};
  dvec2 b{50 + size, 50 - size};
  dvec2 c{50 + size, 50 + size};
  dvec2 d{50 - size, 50 + size};

  planes.emplace_back(Plane({1, 1}, a, size * 2.9));
  planes.emplace_back(Plane({-1, 1}, b, size * 2.9));
  planes.emplace_back(Plane({-1, -1}, c, size * 2.9));
  planes.emplace_back(Plane({1, -1}, d, size * 2.9));
  return planes;
}

int main() {
  unsigned int NUM_PARTICLES = 1000;
  const unsigned int POINT_RADIUS = 8;
  const double DT = 0.002;
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

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> realDist(-1, 1);
  std::vector<Particle> particles;
  for (int i = 0; i < static_cast<int>(NUM_PARTICLES); i++) {
    particles.emplace_back(gen, realDist);
  }

  std::vector<Plane> planes = diamondPlanes(20);

  std::vector<Receiver> receivers;
  receivers.emplace_back(60, 60, 2);

  double time = 0;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(1, 1, 1);
    for (const Plane &plane : planes) {
      glVertex2f(plane.end_a[0], plane.end_a[1]);
      glVertex2f(plane.end_b[0], plane.end_b[1]);
    }
    glEnd();

    for (Particle &p : particles) {
      p.move(DT, planes);
      p.check_receiver_collision(time, receivers);
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle &p) { return !p.alive; }),
                    particles.end());

    glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Particle &p : particles) {
      float r = static_cast<float>((p.x[0] - 30.0) / 40.0);
      float g = static_cast<float>((p.x[1] - 30.0) / 40.0);
      float b = static_cast<float>((r + g) / 2.0);
      glColor3f(r, g, b);
      glVertex2f(p.x[0], p.x[1]);
    }
    glEnd();

    // glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Receiver &r : receivers) {
      glPointSize(r.size * 2);
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex2f(r.x[0], r.x[1]);
    }
    glEnd();

    glfwSwapBuffers(window);

    glfwPollEvents();
    time += DT;
  }

  glfwTerminate();
  return 0;
}
