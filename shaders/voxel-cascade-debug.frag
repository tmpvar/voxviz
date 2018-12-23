#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"

out vec4 outColor;
flat in vec3 color;
flat in int vlevel;
flat in vec3 vposition;

uniform int total_levels;

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
  uvec3 pos = uvec3(vposition);
  uint idx = pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER;
  Cell cell = levels[vlevel].cells[idx];
  if (cell.state > 0) {
    outColor = vec4(color, 1.0);
  } else {
    outColor = vec4(.25, 0.25, 0.25, 1.0);
  }
  //outColor = vec4(color, 1.0);
}
