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

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL=1 NOMINMAX=1)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/libuv)

add_subdirectory(shaders)

include_directories(
  include
  ${OPENGL_INCLUDE_DIR}
  ${OPENCL_INCLUDE_DIRS}
  ${OpenCL_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  deps/glm/
  deps/libuv/include
  deps/imgui
  deps/imgui/examples
  deps/stb
)



ENABLE_TESTING()
ADD_SUBDIRECTORY( test )

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
  include/voxviz/aabb.h
  include/shadowmap.h
	include/volume.h
  include/volume-manager.h
	include/blue-noise.h
	include/scene.h
	include/obb.h
  include/splats.h

  include/parser/vzd/vzd.h
  include/parser/magicavoxel/vox.h

  include/collision/aabb-obb.h
  include/greedy-mesher.h
  include/model.h

  src/fullscreen-surface.cpp
  include/fullscreen-surface.h

  ${SHADER_FILES}

  #imgui
  deps/imgui/imgui.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/examples/imgui_impl_glfw.cpp
  deps/imgui/examples/imgui_impl_opengl3.cpp

	#stb
	deps/stb/stb_image.h
)

add_compile_definitions(STB_IMAGE_IMPLEMENTATION)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT voxviz)

source_group("shaders" FILES ${SHADER_FILES})

add_dependencies(voxviz shaders_out)
add_definitions(-DGPU_HOST=1)


target_link_libraries(
  voxviz
  glfw
  uv_a
  ${OPENGL_gl_LIBRARY}
  ${OPENCL_LIBRARIES}
 )

file(COPY img DESTINATION ${CMAKE_BINARY_DIR})
