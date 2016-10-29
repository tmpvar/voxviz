cmake_minimum_required(VERSION 3.0)
project(shaders)

add_custom_command(
  OUTPUT
    built.h
    cache-buster.h # ensure this always runs
  COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/convert.js ${CMAKE_CURRENT_BINARY_DIR}/built.h
)

set(GENERATED_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(shaders_out DEPENDS built.h)
