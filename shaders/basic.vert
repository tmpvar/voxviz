#version 150 core

in vec3 position;
out vec2 uv;

void main() {
  uv = (1.0 + position.xy) / 2.0;
  gl_Position = vec4(position, 1.0);
}
