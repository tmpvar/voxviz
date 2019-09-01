#include "core.h"

uint brick_morton_idx(uvec3 pos) {
  uint ret = 0;

  // bit interleave: zyxzyxzyxzyx

  // x
  ret |= ((pos.x & (1 << 0)) >> 0) << 0;
  ret |= ((pos.x & (1 << 1)) >> 1) << 3;
  ret |= ((pos.x & (1 << 2)) >> 2) << 6;
  ret |= ((pos.x & (1 << 3)) >> 3) << 9;

  // y
  ret |= ((pos.y & (1 << 0)) >> 0) << 1;
  ret |= ((pos.y & (1 << 1)) >> 1) << 4;
  ret |= ((pos.y & (1 << 2)) >> 2) << 7;
  ret |= ((pos.y & (1 << 3)) >> 3) << 10;

  // z
  ret |= ((pos.z & (1 << 0)) >> 0) << 2;
  ret |= ((pos.z & (1 << 1)) >> 1) << 5;
  ret |= ((pos.z & (1 << 2)) >> 2) << 8;
  ret |= ((pos.z & (1 << 3)) >> 3) << 11;

  return ret;
}
