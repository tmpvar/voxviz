cmake_minimum_required(VERSION 3.0)
project(VoxelViz)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

# find opencl on osx/linux
IF(UNIX)
	set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/cmake-findopencl/cmake")
	set(CMAKE_CXX_FLAGS "-std=c++0x -g -march=native -msse4.2 -mavx")
ENDIF()

IF(WINDOWS)
	set_target_properties(voxviz PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /EHsc")
ENDIF()

FIND_PACKAGE(OpenCL REQUIRED)

add_subdirectory(deps/glfw)
add_subdirectory(deps/glm)
add_subdirectory(deps/libuv)
add_subdirectory(shaders)

include_directories(
  include
  ${OPENGL_INCLUDE_DIR}
  ${OPENCL_INCLUDE_DIRS}
  ${OpenCL_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GLM_INCLUDE_DIR}
  deps/libuv/include
)

add_executable(
  voxviz
  src/aabb.cpp
  src/main.cpp
  src/glad.c
  src/compute.cpp
  src/clu.cpp
  src/volume.cpp
  src/gl-wrap.cpp
  include/raytrace.h
  include/gl-wrap.h
  include/camera-orbit.h
  include/camera-free.h
  include/clu.h
  include/compute.h
  include/core.h
  include/volume.h
  include/aabb.h

  src/fullscreen-surface.cpp
  include/fullscreen-surface.h
)

add_dependencies(voxviz shaders_out)

target_link_libraries(
  voxviz
  glfw
  glm
  uv_a
  ${OPENGL_gl_LIBRARY}
  ${OPENCL_LIBRARIES}
  ${OpenCL_LIBRARIES}
)
target_link_libraries(voxviz )
