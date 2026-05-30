#include "Simulation.hpp"
#include <algorithm>
#include <utility>

Simulation::Simulation(Scene scene_in, SimConfig cfg_in,
                       Atmosphere atmosphere_in)
    : scene(std::move(scene_in)), cfg(cfg_in), atmosphere(atmosphere_in),
      air(atmosphere) {
  const double c = atmosphere.sound_speed();

  for (Emitter &em : scene.emitters) {
    auto emitted = em.emit(cfg.num_particles, c);
    particles.insert(particles.end(), emitted.begin(), emitted.end());
  }
}

void Simulation::step(double dt) {
  const bool offline = cfg.fidelity == SimConfig::Fidelity::Offline;

  for (Particle &p : particles) {
    p.move(dt, scene.planes);

    // only applies the simple air absorption for offline running
    if (!offline)
      air.decay_step(p.energies, p.vel * dt);

    p.check_receiver_collision(time, scene.receivers, offline ? &air : nullptr);
    p.check_energy();
  }

  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](const Particle &p) { return !p.alive; }),
                  particles.end());

  time += dt;
}

void Simulation::run_offline() {
  while (!particles.empty() && time < cfg.max_time) {
    step(cfg.dt);
  }
}
