#version 430 core

in vec3 position;
uniform mat4 VP;

void main() {
  mat4 inv = inverse(VP);
  gl_Position = vec4(position, 1.0);
}
