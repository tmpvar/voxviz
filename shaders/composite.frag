#version 330 core

out vec4 outColor;
in vec2 uv;
uniform sampler2D color;

void main() {

  outColor = texture(color, uv);
  //outColor = vec4(abs(uv), 0.5, 1.0);
}
