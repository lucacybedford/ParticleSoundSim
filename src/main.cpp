#include <Eigen/Core>
#include <Eigen/Dense>
#include <cmath>
#include <functional>
#include <map>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <random>
#include <vector>

using Eigen::Vector2d;

struct Plane {
  Vector2d n;
  Vector2d p;
  double l;
  Plane(Vector2d normal, Vector2d point, double length)
      : n(normal.normalized()), p(point), l(length) {}
};

struct Point {
  int MAX_ITERATIONS = 5;
  double dt = 0.016; // 1/60th of a second (duration of one timestep)
  double vel = 10;
  Vector2d x{50, 50};
  Vector2d v{0.1, 0.1};
  Point(double vx, double vy, double vel) : v(vx, vy), vel(vel) {
    v = v.normalized() * vel;
  };
  Point() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> realDist(-1, 1);
    v(0) = realDist(gen);
    v(1) = realDist(gen);
    v = v.normalized() * vel;
  };
  void hit(Plane &plane) {
    // vel *= 0.8;
    v = v.normalized() * vel;
  }
  void update(unsigned int r, std::vector<Plane> &planes) {
    double remaining_dt = dt;

    int iteration = 0;
    while (remaining_dt > 0 && iteration < MAX_ITERATIONS) {
      Vector2d x_new = x + v * remaining_dt;
      std::map<double, std::reference_wrapper<Plane>> planeDistances;

      for (Plane &p : planes) {
        double t = p.n.dot(p.p - x) / p.n.dot(x_new - x);

        if (t > 1 || t < 0) {
          continue;
        } else {
          planeDistances.emplace(t, std::ref(p));
        }
      }

      if (planeDistances.size() > 0) {
        auto &[minT, collisionPlaneRef] = *planeDistances.begin();
        Plane &collisionPlane = collisionPlaneRef.get();

        Vector2d x_hit = x + minT * (x_new - x);
        x = x_hit + collisionPlane.n * 0.00001;
        hit(collisionPlane);
        Vector2d v_new = v - 2 * (v.dot(collisionPlane.n)) * collisionPlane.n;
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

  Vector2d a{50 - size, 50 - size};
  Vector2d b{50 + size, 50 - size};
  Vector2d c{50 + size, 50 + size};
  Vector2d d{50 - size, 50 + size};

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

  std::vector<Plane> planes = diamondPlanes(10);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(1, 1, 1);
    for (const Plane &plane : planes) {
      Vector2d tangent(-plane.n(1), plane.n(0));
      double half_len = plane.l / 2;
      Vector2d a = plane.p - tangent * half_len;
      Vector2d b = plane.p + tangent * half_len;
      glVertex2f(a(0), a(1));
      glVertex2f(b(0), b(1));
    }
    glEnd();

    glPointSize(POINT_RADIUS * 2);
    glBegin(GL_POINTS);
    for (Point &p : points) {
      p.update(POINT_RADIUS * 0.5, planes);
      float vx = p.dt * p.x(0);
      float vy = p.dt * p.x(1);
      glColor3f(std::abs(vx), std::abs(vy), vx + vy);
      glVertex2f(p.x(0), p.x(1));
    }
    glEnd();

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
