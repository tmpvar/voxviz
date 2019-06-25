#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 translation;
layout (location = 2) in uint32_t in_brick_index;

uniform mat4 MVP;

out vec3 brickSurfacePos;
flat out vec3 brickTranslation;

// flat means "do not interpolate"
flat out uint32_t brick_index;

void main() {
  brickTranslation = translation;
  brickSurfacePos = position;
  brick_index = in_brick_index;
  gl_Position = MVP * vec4(position + translation, 1.0);
}
