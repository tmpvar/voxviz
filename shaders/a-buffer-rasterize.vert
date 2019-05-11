#version 430 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "../include/core.h"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 translation;

uniform mat4 MVP;

out vec3 brickSurfacePos;
out float instance;
flat out vec3 brickTranslation;

void main() {
  brickTranslation = translation;
  brickSurfacePos = position;
  instance = float(gl_InstanceID);
  gl_Position = MVP * vec4(position + translation, 1.0);
}
