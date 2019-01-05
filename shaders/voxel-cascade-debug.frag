#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"

out vec4 outColor;
flat in vec3 color;
flat in int vlevel;
flat in vec3 vposition;
in vec3 world_pos;

uniform vec3 center;
uniform Cell *cascade_index;

layout (std430, binding=1) buffer cascade_slab
{
  SlabEntry entry[];
};


void main() {
  ivec3 pos = ivec3(vposition);
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
   outColor = vec4(1, 0, 1, 1.0);
    return;
  }

  if (voxel_cascade_get(cascade_index, vlevel, pos)) {
    outColor = vec4(color, 1.0);
    gl_FragDepth = distance(world_pos, center) / MAX_DISTANCE;
  } else {
    discard;
    outColor = vec4(.15, 0.15, 0.15, 1.0);
  }
  //outColor = vec4(color, 1.0);
}
