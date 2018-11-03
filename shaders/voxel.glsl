#include "../include/core.h"

bool voxel_get(uint *vol, ivec3 pos) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return false;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  return vol[idx] > 0;

  // uint word = idx / VOXEL_WORD_BITS;
  // uint bit = idx % VOXEL_WORD_BITS;
  // uint mask = (1 << bit);
  // return (vol[word] & mask) != 0;
}

void voxel_set(uint *vol, ivec3 pos, bool v) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  vol[idx] = v ? 1 : 0;
  // uint word = idx / VOXEL_WORD_BITS;
  // uint bit = idx % VOXEL_WORD_BITS;
  // uint cur = vol[word];
  // uint mask = (1 << bit);

  // vol[word] |= mask;// v ? (cur | mask) : (cur & ~mask);
}