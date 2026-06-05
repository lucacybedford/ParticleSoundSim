#include "Particle.hpp"
#include "AirAbsorption.hpp"
#include "Receiver.hpp"
#include <glm/geometric.hpp>
#include <limits>
#include <numeric>

Particle::Particle(std::mt19937 &gen,
                   std::uniform_real_distribution<double> &angDist,
                   dvec2 &position, double speed)
    : vel(speed), x(position) {
  v[0] = cos(angDist(gen));
  v[1] = sin(angDist(gen));
  v = glm::normalize(v) * vel;
}

void Particle::absorb() { alive = false; }

double Particle::check_energy() {
  double energy_sum = std::accumulate(energies.begin(), energies.end(), 0.0);
  if (energy_sum < energy_threshold) {
    absorb();
  }
  return energy_sum;
}

void Particle::hit(Plane &plane) {
  v = glm::normalize(v) * vel;
  // applies the material's absorption coefficients for each frequency band
  for (size_t i = 0; i < energies.size(); i++) {
    energies[i] *= (1 - plane.absorption[i]);
  }
}

void Particle::check_receiver_collision(double time,
                                        std::vector<Receiver> &receivers,
                                        const AirAbsorption *summation) {
  for (Receiver &rec : receivers) {
    float dist = glm::distance(rec.x, x);
    if (dist <= rec.size) {
      std::array<double, 8> e = energies;
      // apply summation method of offline
      if (summation)
        summation->attenuate_total(e, vel * time);
      rec.receive(time, e);
      absorb();
    }
  }
};

void Particle::move(double dt, std::vector<Plane> &planes) {
  double remaining_dt = dt;

  // uses max iterations for calculating particle position after wall
  // interaction
  int iteration = 0;
  while (remaining_dt > 0 && iteration < MAX_ITERATIONS) {
    dvec2 x_new = x + v * remaining_dt;
    double minT = std::numeric_limits<double>::max();
    Plane *closestPlane = nullptr;

    for (Plane &p : planes) {
      // component of particle's motion perpendicular to the wall
      double denom = glm::dot(p.n, x_new - x);
      if (std::abs(denom) < 1e-12)
        continue;
      // fraction along the path where it crosses the wall's infinite plane
      double t = glm::dot(p.n, p.p - x) / denom;
      if (t < 0 || t > 1 || t >= minT)
        continue;
      // verify the crossing is on the finite segment, not the infinite plane
      dvec2 x_hit = x + t * (x_new - x);
      double dist = glm::dot(x_hit - p.p, p.tangent);
      if (std::abs(dist) > p.l / 2)
        continue;
      minT = t;
      closestPlane = &p;
    }

    if (closestPlane) {
      // reposition particle just off the wall surface
      dvec2 x_hit = x + minT * (x_new - x);
      x = x_hit + closestPlane->n * 0.00001;
      hit(*closestPlane);
      // adjust velocity
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
