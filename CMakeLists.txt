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

add_executable(ParticleSoundSim src/main.cpp src/Plane.cpp src/Particle.cpp src/Receiver.cpp)

target_link_libraries(ParticleSoundSim PRIVATE glm::glm glfw OpenGL::GL)
target_include_directories(ParticleSoundSim PRIVATE include)
