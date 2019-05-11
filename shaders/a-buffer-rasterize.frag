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
uniform mat4 M;

in vec3 brickSurfacePos;
flat in vec3 brickTranslation;
in float instance;
out vec4 outColor;

uint valueIndex(uvec3 pos, uvec3 dims) {
  return pos.x + pos.y * dims.x + pos.z * dims.x * dims.y;
}

void main() {
  uint screen_idx = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * dims.x;

  outColor = vec4(gl_FragCoord.xy/vec2(dims), 0.0, 1.0);

  uint depth_idx = atomicAdd(abuffer_index[screen_idx], 1);

  uvec3 abuffer_dims = uvec3(dims, ABUFFER_MAX_DEPTH_COMPLEXITY);
  uint cell_idx = valueIndex(
      uvec3(gl_FragCoord.xy, depth_idx),
      abuffer_dims
  );


  //abuffer_value[cell_idx].surface_position = vec4(brickSurfacePos, gl_FragCoord.w);
  //ABufferCell cell;
  //cell.volume_idx = 1;
  //cell.brick_idx = 2;
  abuffer_value[cell_idx].brick_idx = uint(instance);
  abuffer_value[cell_idx].depth = distance(eye, brickSurfacePos + brickTranslation);
  abuffer_value[cell_idx].surface_position = brickSurfacePos + brickTranslation;

  outColor = vec4(float(depth_idx) / ABUFFER_MAX_DEPTH_COMPLEXITY);
  //outColor = vec4(1.0, instance / (32.0 * 32.0 * 32.0), 0.0, 1.0);
  // return;
  if (depth_idx >= ABUFFER_MAX_DEPTH_COMPLEXITY) {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
    return;
  }
  //
  outColor = vec4((brickSurfacePos + brickTranslation) / 32.0, 1.0);//float(depth_idx)/float(ABUFFER_MAX_DEPTH_COMPLEXITY));
}
