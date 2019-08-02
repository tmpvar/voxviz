cmake_minimum_required(VERSION 3.0)
project(shaders)

# Dummy output which is never actually produced. Anything that depends on
# this will always be rebuilt.
add_custom_command(
	OUTPUT always_rebuild
	COMMAND cmake -E echo
)

add_custom_command(
  OUTPUT
    built.h
  COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/convert.js -o ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS always_rebuild
)

set(GENERATED_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(shaders_out ALL DEPENDS built.h)
