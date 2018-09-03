#version 430 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 iCenter;
layout (location = 2) in float *iBufferPointer;

uniform mat4 MVP;
uniform uvec3 dims;
uniform mat4 model;
uniform vec3 eye;

uniform vec3 tmpCenter;

out vec3 rayOrigin;
out vec3 brickOrigin;
out vec3 center;
out vec3 invEye;
out vec3 rayDir;
// flat means "do not interpolate"
flat out float *volumePointer;

void main() {
  vec3 hd = vec3(dims) / 2.0;
  vec3 pos = position * hd;

  rayOrigin = pos;
  brickOrigin = pos + hd;

  volumePointer = iBufferPointer;
  gl_Position = MVP * vec4(pos, 1.0);
}
