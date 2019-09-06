#version 450 core
#extension GL_NV_gpu_shader5: enable

in vec3 position;

uniform mat4 viewProjection;
uniform mat4 model;
uniform uint totalTriangles;

out vec3 vert_pos;

#include "hsl.glsl"

void main() {
  vec4 hpos = vec4(position, 1.0);
  vec4 tpos = model * hpos;
  vert_pos = tpos.xyz / tpos.w;
  mat4 mvp = viewProjection * model;
  gl_Position = mvp * hpos;
}
