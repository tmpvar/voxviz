#version 430 core



out vec4 outColor;
flat in vec3 color;

uniform int total_levels;

void main() {
  outColor = vec4(color, 1.0);
}
