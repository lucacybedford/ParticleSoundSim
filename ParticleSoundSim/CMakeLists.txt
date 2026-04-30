cmake_minimum_required(VERSION 3.20)
project(ParticleSoundSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 5.0 REQUIRED NO_MODULE)

add_executable(ParticleSoundSim src/main.cpp)

target_link_libraries(ParticleSoundSim PRIVATE Eigen3::Eigen)

# GLFW headers
target_include_directories(ParticleSoundSim PRIVATE
    ${CMAKE_SOURCE_DIR}/include/GLFW/include
)

# GLFW static library (arm64)
target_link_directories(ParticleSoundSim PRIVATE
    ${CMAKE_SOURCE_DIR}/include/GLFW/lib-arm64
)
target_link_libraries(ParticleSoundSim PRIVATE glfw3)

# macOS system frameworks required by GLFW
target_link_libraries(ParticleSoundSim PRIVATE
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)
