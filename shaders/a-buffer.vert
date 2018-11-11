#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 iCenter;
layout (location = 3) in uint id;

uniform mat4 MVP;
uniform uvec3 dims;
uniform vec3 eye;
uniform vec3 center;
out vec3 rayOrigin;
flat out uint brickId;

void main() {
  vec3 pos = position * (vec3(dims) / 2.0);
  rayOrigin = pos + iCenter;
  brickId = id;
  gl_Position = MVP * vec4(rayOrigin, 1.0);
}