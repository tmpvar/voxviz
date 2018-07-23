#version 330 core

out vec4 outColor;
in vec2 uv;
uniform sampler2D color;
uniform float maxDistance;

void main() {
  outColor = texture(color, uv);
  if (outColor.r > 1.0) {
	outColor.r = outColor.r / maxDistance;
  }
}
