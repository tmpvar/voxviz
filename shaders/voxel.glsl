#include "../include/voxel.h"

struct BrickData {
  uint8_t data[BRICK_VOXEL_WORDS];
};

layout (std430) buffer brickBuffer {
  BrickData bricks[];
};

bool voxel_get(uint brick_index, vec3 pos) {
  if (true) {
    uint idx = uint(
      uint(pos.x) +
      uint(pos.y) * BRICK_DIAMETER +
      uint(pos.z) * BRICK_DIAMETER * BRICK_DIAMETER
    );

    uint word = idx / VOXEL_WORD_BITS;
    uint bit = idx % VOXEL_WORD_BITS;
    uint8_t mask = uint8_t(1 << bit);
    return (bricks[brick_index].data[word] & mask) > uint8_t(0);
  } else {
    uint idx = brick_morton_idx(uvec3(pos));
    uint word = idx >> 3;
    uint mask = idx & 8;
    return (bricks[brick_index].data[word] & mask) > uint8_t(0);
  }
}

void voxel_set(uint brick_index, ivec3 pos, bool v) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  uint word = uint(float(idx) / float(VOXEL_WORD_BITS));
  uint bit = idx % VOXEL_WORD_BITS;
  uint8_t cur = bricks[brick_index].data[word];
  uint8_t mask = uint8_t(1 << bit);

  bricks[brick_index].data[word] |= uint8_t(v ? (cur | mask) : (cur & ~mask));
}
