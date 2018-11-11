cmake_minimum_required(VERSION 3.0)
project(VoxelViz)


find_package(OpenGL REQUIRED)

# find opencl on osx/linux
IF(UNIX)
	set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/cmake-findopencl/cmake")
	set(CMAKE_CXX_FLAGS "-std=c++0x -g -march=native -msse4.2 -mavx")
ENDIF()

IF(WINDOWS)
	set_target_properties(voxviz PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /EHsc")
ENDIF()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glm)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL=1)
add_subdirectory(deps/libuv)

SET(qu3e_build_demo OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/qu3e)

add_subdirectory(shaders)

include_directories(
  include
  ${OPENGL_INCLUDE_DIR}
  ${OPENCL_INCLUDE_DIRS}
  ${OpenCL_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GLM_INCLUDE_DIR}
	deps/qu3e/src
  deps/libuv/include
  deps/imgui
  deps/imgui/examples
  deps/catch2/include
)

file(GLOB_RECURSE SHADER_FILES shaders/*.frag shaders/*.vert shaders/*.cl shaders/*.comp shaders/*.glsl)

add_executable(
  voxviz
  src/aabb.cpp
  src/main.cpp
  src/glad.c
  src/brick.cpp
  src/gl-wrap.cpp
  include/raytrace.h
  include/gl-wrap.h
  include/camera-orbit.h
  include/camera-free.h
  include/core.h
  include/brick.h
  include/aabb.h
  include/shadowmap.h
	include/volume.h
  include/volume-manager.h

  include/parser/vzd/vzd.h
  include/parser/magicavoxel/vox.h

  include/collision/aabb-obb.h

  src/fullscreen-surface.cpp
  include/fullscreen-surface.h
  include/a-buffer.h

  ${SHADER_FILES}

  #imgui
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/examples/imgui_impl_glfw.cpp
  deps/imgui/examples/imgui_impl_opengl3.cpp
)

source_group("shaders" FILES ${SHADER_FILES})

add_dependencies(voxviz shaders_out)

target_link_libraries(
  voxviz
  glfw
  glm
  uv_a
	qu3e
  ${OPENGL_gl_LIBRARY}
  ${OPENCL_LIBRARIES}
 )

file(GLOB TEST_FILES test/*)

add_executable(
  voxviz_test
  ${TEST_FILES}
  src/aabb.cpp
  src/glad.c
  src/brick.cpp
  src/gl-wrap.cpp
  include/raytrace.h
  include/gl-wrap.h
  include/camera-orbit.h
  include/camera-free.h
  include/core.h
  include/brick.h
  include/aabb.h
  include/shadowmap.h
  include/volume.h
  include/volume-manager.h

  include/parser/vzd/vzd.h
  include/parser/magicavoxel/vox.h

  src/fullscreen-surface.cpp
  include/fullscreen-surface.h

  ${SHADER_FILES}

  #imgui
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/examples/imgui_impl_glfw.cpp
  deps/imgui/examples/imgui_impl_opengl3.cpp
)

target_compile_definitions(voxviz_test PRIVATE "TESTING=1")

target_link_libraries(
  voxviz_test
  glfw
  glm
  uv_a
  qu3e
  ${OPENGL_gl_LIBRARY}
  ${OPENCL_LIBRARIES}
)
