#version 150 core

in vec3 position;
uniform mat4 MVP;
uniform uvec3 dims;
uniform vec3 eye;
uniform vec3 center;
uniform mat4 depthBiasMVP;

out vec3 rayOrigin;
out vec4 shadowCoord;


void main() {
  vec3 pos = position * (vec3(dims) / 2.0);
  rayOrigin = pos + center;
  shadowCoord = depthBiasMVP * vec4(eye, 1.0);
  gl_Position = MVP * vec4(rayOrigin, 1.0);
}
