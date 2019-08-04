cmake_minimum_required(VERSION 3.14)
project(VoxelViz)
find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(WINDOWS)
	set_target_properties(voxviz PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /EHsc")
ENDIF()

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

# FlatBuffers
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/flatbuffers-1.11.0)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL=1)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/libuv)

SET(qu3e_build_demo OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/qu3e)

add_subdirectory(shaders)

include_directories(
  include
  ${OPENGL_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  deps/glm/
	deps/qu3e/src
  deps/libuv/include
  deps/imgui
  deps/imgui/examples
  deps/stb
	deps/flatbuffers-1.11.0/include
)

include(deps/flatbuffers-1.11.0/CMake/BuildFlatBuffers.cmake)
SET(FLATBUFFERS_FLATC_EXECUTABLE OFF CACHE BOOL "" FORCE)
SET(FLATBUFFERS_FLATC_SCHEMA_EXTRA_ARGS --js)
build_flatbuffers(
	"${CMAKE_CURRENT_SOURCE_DIR}/proto/renderpipe.fbs"
	""
	"renderpipe_proto"
	""
	"${CMAKE_CURRENT_SOURCE_DIR}/proto"
	""
	""
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
  include/aabb.h
  include/shadowmap.h
	include/volume.h
  include/volume-manager.h
	include/blue-noise.h
	include/scene.h
	include/obb.h
  include/splats.h
	include/renderpipe.h

  include/parser/vzd/vzd.h
  include/parser/magicavoxel/vox.h

  include/collision/aabb-obb.h

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

	# proto
	proto
)

add_compile_definitions(STB_IMAGE_IMPLEMENTATION)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT voxviz)

source_group("shaders" FILES ${SHADER_FILES})

add_dependencies(voxviz shaders_out renderpipe_proto)
add_definitions(-DGPU_HOST=1)


target_link_libraries(
  voxviz
  glfw
  uv_a
	qu3e
  ${OPENGL_gl_LIBRARY}
 )

file(COPY img DESTINATION ${CMAKE_BINARY_DIR})
