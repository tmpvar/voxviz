cmake_minimum_required(VERSION 3.0)
project(VoxelViz)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

add_subdirectory(deps/glfw)
include_directories(include ${OPENGL_INCLUDE_DIR})

add_executable(
  voxviz
  src/glad.c
  src/main.c
)

target_link_libraries(voxviz glfw ${OPENGL_gl_LIBRARY})
target_link_libraries(voxviz )
