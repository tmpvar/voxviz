#version 450 core
in vec3 position;

uniform mat4 mvp;
uniform uint totalVerts;
uniform uint totalTriangles;
out vec4 color;

#include "hsl.glsl"

void main() {

  // color = vec4(
  //   hsl(vec3(float(gl_VertexID)/float(totalVerts), 0.9, 0.45)),
  //   1.0
  // );


  gl_Position = mvp * vec4(position, 1.0);
}
