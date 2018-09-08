#version 430 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "../include/core.h"

layout (location = 0) in vec3 position;
layout (location = 1) in float *iBufferPointer;

uniform mat4 MVP;
uniform vec3 eye;

out vec3 rayOrigin;
out vec3 brickOrigin;
out vec3 invEye;
out vec3 rayDir;
// flat means "do not interpolate"
flat out float *volumePointer;

void main() {
  vec3 hd = vec3(BRICK_RADIUS);
  vec3 pos = position;

  rayOrigin = pos;
  brickOrigin = pos + BRICK_RADIUS;

  volumePointer = iBufferPointer;
  gl_Position = MVP * vec4(pos, 1.0);
}
