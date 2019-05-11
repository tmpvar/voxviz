#version 450 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"

layout (std430, binding=1) buffer aBufferIndexSlab
{
  uint abuffer_index[];
};

layout (std430, binding=2) buffer aBufferValueSlab
{
   ABufferCell abuffer_value[];
};

uniform uvec2 dims;
uniform vec3 eye;

out vec4 outColor;

uint valueIndex(uvec3 pos, uvec3 dims) {
  return pos.x + pos.y * dims.x + pos.z * dims.x * dims.y;
}

void main() {
  outColor = vec4(gl_FragCoord.xy / vec2(dims), 0.0, 1.0);


  uint total_cells = abuffer_index[uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * dims.x];
  if (total_cells == 0) {
    return;
  }
  uint idx = valueIndex(uvec3(gl_FragCoord.xy, 0), uvec3(dims, ABUFFER_MAX_DEPTH_COMPLEXITY));

  // outColor = vec4(
  //   float(uint(abuffer_value[idx].brick_idx) % 32) / 32.0,
  //   float(uint(abuffer_value[idx].brick_idx / float(32)) % 32) / 32.0,
  //   float(uint(abuffer_value[idx].brick_idx / float(32*32)) % 32) / 32.0,
  //   1.0
  // );

  outColor = vec4(abuffer_value[idx].surface_position, 1.0);

  //outColor = vec4(abuffer_value[idx].depth/100.0);
}
