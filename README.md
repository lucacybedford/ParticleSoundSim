# Proposal

My proposed project aims to develop a computationally efficient simulation for approximating sound propagation in enclosed spaces, inspired by computational fluid dynamics (CFD) but implemented using a simplified particle-based approach. The objective is to model how sound waves interact with complex room geometries while significantly reducing computational cost compared to full wave-based or CFD acoustic simulations.

The system will simulate one or more sound sources emitting discrete sound particles that propagate through a virtual environment. These particles will interact with surfaces while taking into account material acoustic properties such as absorption, reflection, and transmission coefficients. The model will account for room geometry, surface materials, travel distance (for time delay and volume reduction), reflections (echo and reverberation), and partial transmission through walls and objects. One or more virtual microphones will record the accumulated particle interactions to reconstruct the perceived sound signal at specific locations.

A key focus of the project will be optimisation, with the goal of achieving real-time or near real-time performance suitable for applications in games, film production, and virtual environments where realistic spatial audio is required. Performance improvements may include parallelisation and efficient collision handling.

As a stretch goal, a more accurate offline simulation mode could be used to generate large acoustic datasets. These datasets may then be used to train a neural network capable of approximating environmental audio effects, enabling fast generation in real-time systems.

The project will be implemented in C++, with visualisation and validation tools developed using OpenGL to analyse particle behaviour, room geometry and construction, physical interactions, and microphone placement.
