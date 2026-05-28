#include "Particle.hpp"
#include "Receiver.hpp"
#include <glm/geometric.hpp>
#include <limits>
#include <numeric>

Particle::Particle(std::mt19937 &gen,
                   std::uniform_real_distribution<double> &angDist,
                   dvec2 &position)
    : x(position) {
  v[0] = cos(angDist(gen));
  v[1] = sin(angDist(gen));
  v = glm::normalize(v) * vel;
}

void Particle::absorb() { alive = false; }

void Particle::check_energy() {
  double energy_sum = std::accumulate(energies.begin(), energies.end(), 0.0);
  if (energy_sum < energy_threshold) {
    absorb();
  }
}

void Particle::hit(Plane &plane) {
  v = glm::normalize(v) * vel;
  for (size_t i = 0; i < energies.size(); i++) {
    energies[i] *= (1 - plane.absorption[i]);
  }
}

void Particle::check_receiver_collision(double time,
                                        std::vector<Receiver> &receivers) {
  for (Receiver &r : receivers) {
    float dist = glm::distance(r.x, x);
    if (dist <= r.size) {
      r.receive(time, energies);
      absorb();
    }
  }
};

void Particle::move(double dt, std::vector<Plane> &planes) {
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
