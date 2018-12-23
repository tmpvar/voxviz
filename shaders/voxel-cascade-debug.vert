#version 430 core

#include "../include/core.h"
#include "hsl2rgb.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 translation;
layout (location = 2) in int level;


uniform mat4 mvp;
uniform vec3 center;
uniform int total_levels;

flat out vec3 color;
flat out int vlevel;
flat out vec3 vposition;

void main() {
  float cellSize = float(1 << (level + 1));

  color = hsl2rgb(float(level)/float(total_levels), 0.9, .4);
  vposition = translation;
  vlevel = level;
  gl_Position = mvp * vec4((position + translation - BRICK_RADIUS) * cellSize, 1.0);
}
