#include "../include/core.h"

struct BrickData {
  uint32_t data[BRICK_VOXEL_WORDS];
};

layout (std430) buffer brickBuffer {
  BrickData bricks[];
};

bool voxel_get(uint brick_index, vec3 pos) {
  uint idx = uint(
    uint(pos.x) +
    uint(pos.y) * BRICK_DIAMETER +
    uint(pos.z) * BRICK_DIAMETER * BRICK_DIAMETER
  );

  uint word = idx / VOXEL_WORD_BITS;
  uint bit = idx % VOXEL_WORD_BITS;
  uint mask = (1 << bit);
  return (bricks[brick_index].data[word] & mask) > 0;
}

void voxel_set(uint brick_index, ivec3 pos, bool v) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  uint word = uint(float(idx) / float(VOXEL_WORD_BITS));
  uint bit = idx % VOXEL_WORD_BITS;
  uint cur = bricks[brick_index].data[word];
  uint mask = (1 << bit);

  bricks[brick_index].data[word] |= v ? (cur | mask) : (cur & ~mask);
}
