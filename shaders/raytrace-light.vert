#version 330 core

in vec3 position;
uniform mat4 MVP;
uniform uvec3 dims;
uniform vec3 center;

out vec3 rayOrigin;

void main() {
  vec3 pos = position * (vec3(dims) / 2.0);
  rayOrigin = pos + center;
  gl_Position = MVP * vec4(rayOrigin, 1.0);
}
