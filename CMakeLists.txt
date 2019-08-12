cmake_minimum_required(VERSION 3.0)
project(VoxelViz)
find_package(OpenGL REQUIRED)
# TODO: using CONFIG REQUIRED here breaks and im not sure how to fix it
# Could not find a package configuration file provided by "openvdb" with any
#   of the following names:
#
#     openvdbConfig.cmake
#     openvdb-config.cmake
#
#   Add the installation prefix of "openvdb" to CMAKE_PREFIX_PATH or set
set(VCPKG_APPLOCAL_DEPS ON CACHE BOOL "" FORCE)
find_package(OpenEXR REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(TBB CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(OpenVR REQUIRED)


IF(WINDOWS)
	set_target_properties(voxviz PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS /EHsc")
	add_definitions(-D_WIN32 -DNOMINMAX)
ENDIF()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL=1)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/libuv)

SET(qu3e_build_demo OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/qu3e)

add_subdirectory(shaders)

include_directories(
  include
  ${OPENGL_INCLUDE_DIR}
  ${OPENCL_INCLUDE_DIRS}
  ${OpenCL_INCLUDE_DIRS}
	# TODO: use a cmake var here.. I think vcpkg is stomping on the old value
  build/
  deps/glm/
	deps/qu3e/src
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
  include/aabb.h
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

target_include_directories(voxviz PRIVATE
	${OPENVDB_INCLUDE_DIRS}
	${TBB_INCLUDE_DIRS}
  ${OPENVR_INCLUDE_DIRS}
)

add_compile_definitions(STB_IMAGE_IMPLEMENTATION)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT voxviz)

source_group("shaders" FILES ${SHADER_FILES})

add_dependencies(voxviz shaders_out)
add_definitions(-DGPU_HOST=1)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

message("SNAPPY_LIBRARIES: ${SNAPPY_LIBRARIES}")

target_link_libraries(
  voxviz
  glfw
  uv_a
	qu3e
  ${OPENGL_gl_LIBRARY}
  ${OPENCL_LIBRARIES}
	${OPENVDB_LIBRARIES}
	TBB::tbb
	${OpenEXR_HALF_LIBRARY}
  ${OPENVR_LIBRARIES}
 )

file(COPY img DESTINATION ${CMAKE_BINARY_DIR})
