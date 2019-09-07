#version 450 core
#extension GL_NV_gpu_shader5: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 viewProjection;
uniform mat4 model;

out vec3 vert_pos;
out vec3 vert_normal;

#include "hsl.glsl"

void main() {
  vec4 hpos = vec4(position, 1.0);
  vec4 tpos = model * hpos;
  vert_pos = tpos.xyz / tpos.w;
  mat4 mvp = viewProjection * model;

  vec4 tnorm = model * vec4(normal, 0.0);
  vert_normal = tnorm.xyz;


  gl_Position = mvp * hpos;
}
