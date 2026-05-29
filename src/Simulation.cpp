#include "Simulation.hpp"
#include <algorithm>
#include <utility>

Simulation::Simulation(Scene scene_in, SimConfig cfg_in)
    : scene(std::move(scene_in)), cfg(cfg_in) {
  // Emit particles from every emitter in the scene (was done inline in main()).
  for (Emitter &em : scene.emitters) {
    auto emitted = em.emit(cfg.num_particles);
    particles.insert(particles.end(), emitted.begin(), emitted.end());
  }
}

void Simulation::step(double dt) {
  for (Particle &p : particles) {
    p.move(dt, scene.planes);
    p.check_receiver_collision(time, scene.receivers);
    p.check_energy();
  }

  // erase-remove the particles that died this step (absorbed or below the
  // energy threshold).
  particles.erase(
      std::remove_if(particles.begin(), particles.end(),
                     [](const Particle &p) { return !p.alive; }),
      particles.end());

  time += dt;
}

void Simulation::run_offline() {
  while (!particles.empty() && time < cfg.max_time) {
    step(cfg.dt);
  }
}
