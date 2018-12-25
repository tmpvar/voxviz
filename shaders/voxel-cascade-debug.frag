#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"

out vec4 outColor;
flat in vec3 color;
flat in int vlevel;
flat in vec3 vposition;

uniform int total_levels;
uniform vec3 center;

struct Cell {
  uint state;
  uint start;
  uint end;
};

struct Level {
  Cell cells[4096];
};

struct SlabEntry {
  uint volume;
  ivec3 brickIndex;
  uint32_t *brickData;
};

layout (std430, binding=0) buffer cascade_index
{
  Level levels[];
};

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

  uint idx = pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER;
  Cell cell = levels[vlevel].cells[idx];
  if (cell.state > 0) {
    outColor = vec4(color, 1.0);
    gl_FragDepth = distance(vposition, center) / 10000.0f;
  } else {
    discard;
    outColor = vec4(.15, 0.15, 0.15, 1.0);
  }
  //outColor = vec4(color, 1.0);
}
