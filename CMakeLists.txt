cmake_minimum_required(VERSION 3.20)
project(ParticleSoundSim)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

# Shared, render-free simulation core. Both apps link against this so they run
# identical physics. Only glm is needed here -- no GLFW/OpenGL.
add_library(core
  src/Plane.cpp
  src/Particle.cpp
  src/Receiver.cpp
  src/Emitter.cpp
  src/Atmosphere.cpp
  src/AirAbsorption.cpp
  src/Scene.cpp
  src/Simulation.cpp)
target_include_directories(core PUBLIC include)
target_link_libraries(core PUBLIC glm::glm)

# Front-end: the visual simulation (GLFW window + OpenGL rendering).
add_executable(ParticleSoundSim src/app_visual.cpp)
target_link_libraries(ParticleSoundSim PRIVATE core glfw OpenGL::GL)

# Back-end: the headless offline simulation.
add_executable(ParticleSoundSimOffline src/app_offline.cpp)
target_link_libraries(ParticleSoundSimOffline PRIVATE core)
