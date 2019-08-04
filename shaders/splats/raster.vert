#version 430 core

#extension GL_NV_gpu_shader5: enable

#include "../splats.glsl"
#include "../shared/quadric-proj.glsl"

layout (std430) buffer splatInstanceBuffer {
  Splat splats[];
};

uniform mat4 mvp;
uniform vec3 eye;
uniform uvec2 res;
uniform float fov;
flat out vec4 color;
flat out float pointSize;
out vec2 center;

void main() {
  Splat s = splats[gl_VertexID];
  color = s.color;

  vec4 outPos;
  float size;
  quadricProj(s.position.xyz + 0.5, 0.5, mvp, vec2(res)/2.0, outPos, size);
  pointSize = size;
  gl_PointSize = pointSize * s.position.w;
  vec4 pos = mvp * vec4(s.position.xyz, 1.0);
  center = (pos.xy/pos.z);
  gl_Position = pos;
}
