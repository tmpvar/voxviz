#version 430 core

#extension GL_NV_gpu_shader5: enable

#include "splats.glsl"

layout (std430) buffer splatInstanceBuffer {
  Splat splats[];
};

uniform mat4 perspective;
uniform mat4 view;
uniform vec3 eye;
uniform uvec2 res;
uniform float fov;
flat out vec4 color;
flat out float pointSize;
out vec2 center;
//Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
void quadricProj(in vec3 osPosition, in float voxelSize, in mat4 objectToScreenMatrix, in vec2 halfScreenSize, inout vec4 position, inout float pointSize) {
  const vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
  float sphereRadius = voxelSize * 1.732051;
  vec4 sphereCenter = vec4(osPosition.xyz, 1.0);
  mat4 modelViewProj = transpose(objectToScreenMatrix);
  mat3x4 matT = mat3x4( mat3(modelViewProj[0].xyz, modelViewProj[1].xyz, modelViewProj[3].xyz) * sphereRadius);
  matT[0].w = dot(sphereCenter, modelViewProj[0]);
  matT[1].w = dot(sphereCenter, modelViewProj[1]); matT[2].w = dot(sphereCenter, modelViewProj[3]);
  mat3x4 matD = mat3x4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat);
  vec4 eqCoefs = vec4(dot(matD[0], matT[2]), dot(matD[1], matT[2]), dot(matD[0], matT[0]), dot(matD[1], matT[1])) / dot(matD[2], matT[2]);
  vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
  vec2 AABB = sqrt(eqCoefs.xy*eqCoefs.xy - eqCoefs.zw);
  AABB *= halfScreenSize * 2.0f;
  position.xy = outPosition.xy * position.w;
  pointSize = max(AABB.x, AABB.y);
}

void main() {
  Splat s = splats[gl_VertexID];
  color = s.color;
  mat4 MVP = perspective * view;

  vec4 outPos;
  float size;
  quadricProj(s.position.xyz + 0.5, 0.5, MVP, vec2(res)/2.0, outPos, size);
  pointSize = size;
  gl_PointSize = pointSize;// * 1.0/float(min(res.x, res.y)) * id * tan(fov/2.0);
  // gl_PointSize = 2;
  vec4 pos = MVP * s.position;
  center = (pos.xy/pos.z);
  gl_Position = pos;
}
