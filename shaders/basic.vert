#version 150 core

in vec3 position;
uniform mat4 MVP;
uniform ivec3 dims;
out vec4 color;

void main() {
  color = vec4(position/dims, 1.0);
  gl_Position = MVP * vec4(position, 1.0);
}
