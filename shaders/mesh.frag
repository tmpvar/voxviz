#version 150 core

out vec4 outColor;
in vec4 color;
uniform uint totalTriangles;

#include "hsl.glsl"

void main() {
  outColor = vec4(
    hsl(vec3(float(gl_PrimitiveID)/float(totalTriangles), 0.9, 0.45)),
    1.0
  );
}
