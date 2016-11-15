cmake_minimum_required(VERSION 3.0)
project(VoxelViz)

set(CMAKE_CXX_FLAGS "-std=c++0x -g -march=native -msse4.2 -mavx")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

add_subdirectory(deps/glfw)
add_subdirectory(shaders)

include_directories(include ${OPENGL_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(
  voxviz
  src/main.cpp
  src/glad.c
  src/mesher.cpp
  src/raytrace.h
  include/gl-wrap.h
  include/mesher.h
  include/orbit-camera.h
  include/vec.h
)

add_dependencies(voxviz shaders_out)

target_link_libraries(voxviz glfw ${OPENGL_gl_LIBRARY})
target_link_libraries(voxviz )
