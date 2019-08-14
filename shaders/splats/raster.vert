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

void main() {
  Splat s = splats[gl_VertexID];
  float voxelScale = 0.001;
  vec4 outPos;
  float size;
  quadricProj(s.position.xyz, 1, mvp, vec2(res)/2.0, outPos, size);

  gl_PointSize = min(20.0, size);
  gl_PointSize = 2 ;
  vec4 pos = mvp * vec4(s.position.xyz * voxelScale, 1.0);
  gl_Position = pos;
  color = vec4(1.0 - distance(eye, s.position.xyz) / 1000);
  color = vec4(s.position.xyz * voxelScale, 1.0) * 20.0;
}
