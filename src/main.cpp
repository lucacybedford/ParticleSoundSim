#include "glm/geometric.hpp"
#include <glm/glm.hpp>
#include <limits>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <random>
#include <vector>

using glm::dvec2;

struct Plane {
  dvec2 n;
  dvec2 p;
  dvec2 tangent;
  dvec2 end_a;
  dvec2 end_b;
  double l;
  Plane(dvec2 normal, dvec2 point, double length)
      : n(glm::normalize(normal)), p(point), l(length) {
    tangent = {-n[1], n[0]};
    end_a = p - tangent * (l / 2);
    end_b = p + tangent * (l / 2);
  }
};

struct Point {
  static constexpr int MAX_ITERATIONS = 5;
  static constexpr double dt =
      0.016; // 1/60th of a second (duration of one timestep)
  double vel = 10;
  dvec2 x{50, 50};
  dvec2 v{0.1, 0.1};
  Point(double vx, double vy, double speed) : v(vx, vy), vel(speed) {
    v = glm::normalize(v) * vel;
  };
  Point() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> realDist(-1, 1);
    v[0] = realDist(gen);
    v[1] = realDist(gen);
    v = glm::normalize(v) * vel;
  };
  void hit(Plane &plane) { v = glm::normalize(v) * vel; }
  void update(unsigned int r, std::vector<Plane> &planes) {
    double remaining_dt = dt;

    int iteration = 0;
    while (remaining_dt > 0 && iteration < MAX_ITERATIONS) {
      dvec2 x_new = x + v * remaining_dt;
      double minT = std::numeric_limits<double>::max();
      Plane *closestPlane = nullptr;

      for (Plane &p : planes) {
        double denom = glm::dot(p.n, x_new - x);
        if (std::abs(denom) < 1e-12)
          continue;
        double t = glm::dot(p.n, p.p - x) / denom;
        if (t >= 0 && t <= 1 && t < minT) {
          minT = t;
          closestPlane = &p;
        }
      }

      if (closestPlane) {
        dvec2 x_hit = x + minT * (x_new - x);
        x = x_hit + closestPlane->n * 0.00001;
        hit(*closestPlane);
        dvec2 v_new = v - 2 * glm::dot(v, closestPlane->n) * closestPlane->n;
        remaining_dt = remaining_dt * (1 - minT);
        v = v_new;
      } else {
        x = x_new;
        break;
      }
      iteration++;
    }
  }
};

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
  unsigned int NUM_POINTS = 1000;
  unsigned int POINT_RADIUS = 8;
  GLFWwindow *window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  /* Create a windowed mode window and its OpenGL context */
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

  std::vector<Point> points;
  for (int i = 0; i < static_cast<int>(NUM_POINTS); i++) {
    points.emplace_back();
  }

  std::vector<Plane> planes = diamondPlanes(20);

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

    for (Point &p : points) {
      p.update(POINT_RADIUS * 0.5, planes);
    }

    glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Point &p : points) {
      float vx = p.dt * p.x[0];
      float vy = p.dt * p.x[1];
      glColor3f(std::abs(vx), std::abs(vy), vx + vy);
      glVertex2f(p.x[0], p.x[1]);
    }
    glEnd();

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
