#version 150 core

out vec4 outColor;
in vec2 uv;

void main() {
  outColor = vec4(abs(uv), 0.5, 1.0);
}
