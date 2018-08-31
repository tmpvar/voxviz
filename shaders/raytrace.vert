#version 430 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 iCenter;
layout (location = 2) in float *iBufferPointer;

uniform mat4 MVP;
uniform uvec3 dims;
uniform vec3 eye;
uniform mat4 depthBiasMVP;

out vec3 rayOrigin;
out vec4 shadowCoord;
out vec3 npos;

out vec3 center;
// flat means "do not interpolate"
flat out float *volumePointer;

void main() {
  vec3 pos = position * (vec3(dims) / 2.0);
  center = iCenter;
  rayOrigin = pos + iCenter;
  shadowCoord = depthBiasMVP * vec4(eye, 1.0);

  volumePointer = iBufferPointer;
  gl_Position = MVP * vec4(rayOrigin, 1.0);
}
