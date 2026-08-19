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

FetchContent_Declare(pocketfft
  GIT_REPOSITORY https://github.com/mreineck/pocketfft.git
  GIT_TAG cpp
)
FetchContent_MakeAvailable(pocketfft)
add_library(pocketfft INTERFACE)
target_include_directories(pocketfft INTERFACE ${pocketfft_SOURCE_DIR})
target_compile_definitions(pocketfft INTERFACE POCKETFFT_NO_MULTITHREADING)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG 0.2.2
)
FetchContent_MakeAvailable(libsamplerate)

# WAV reader/writer
FetchContent_Declare(dr_libs
  GIT_REPOSITORY https://github.com/mackron/dr_libs.git
  GIT_TAG 47a4f08e777faddf59a8955c4ea84f69f41020d5
  GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(dr_libs)
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE ${dr_libs_SOURCE_DIR})

# Butterworth band-pass
set(IIR1_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(IIR1_BUILD_DEMO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(iir1
  GIT_REPOSITORY https://github.com/berndporr/iir1.git
  GIT_TAG 1.9.5
)
FetchContent_MakeAvailable(iir1)

find_package(OpenGL REQUIRED)

add_library(core
  src/Plane.cpp
  src/Impedance.cpp
  src/Particle.cpp
  src/Receiver.cpp
  src/Emitter.cpp
  src/Atmosphere.cpp
  src/AirAbsorption.cpp
  src/Scene.cpp
  src/Simulation.cpp
  src/RIRBuilder.cpp
  src/Convolver.cpp
  src/ConvolveInput.cpp
  src/Wav.cpp)
target_include_directories(core PUBLIC include)
target_link_libraries(core PUBLIC glm::glm)
target_link_libraries(core PRIVATE pocketfft samplerate dr_libs iir_static)

# visual simulation
add_executable(ParticleSoundSim src/app_visual.cpp)
target_link_libraries(ParticleSoundSim PRIVATE core glfw OpenGL::GL)

# offline simulation
add_executable(ParticleSoundSimOffline src/app_offline.cpp)
target_link_libraries(ParticleSoundSimOffline PRIVATE core)

# convolve a pre-computed RIR with a dry input
add_executable(ParticleSoundSimConvolve src/app_convolve.cpp)
target_link_libraries(ParticleSoundSimConvolve PRIVATE core)

# experiments
add_executable(ParticleSoundSimExperiments src/app_experiments.cpp)
target_link_libraries(ParticleSoundSimExperiments PRIVATE core)
