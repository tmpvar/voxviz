#include "../include/core.h"

#define VOXEL_BRICK_ITERATIONS BRICK_DIAMETER*2 + BRICK_RADIUS

bool voxel_get(uint32_t *vol, ivec3 pos) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return false;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  uint word = uint(float(idx) / float(VOXEL_WORD_BITS));
  uint bit = idx % VOXEL_WORD_BITS;
  uint mask = (1 << bit);
  return (vol[word] & mask) != 0;
}

void voxel_set(uint32_t *vol, ivec3 pos, bool v) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  uint word = uint(float(idx) / float(VOXEL_WORD_BITS));
  uint bit = idx % VOXEL_WORD_BITS;
  uint cur = vol[word];
  uint mask = (1 << bit);

  vol[word] |= v ? (cur | mask) : (cur & ~mask);
}

float voxel_march(uint32_t *volume, in out vec3 pos, vec3 rayDir, out vec3 center, out vec3 normal, out float iterations) {
  pos -= rayDir * 4.0;
  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = (sign(rayDir) * (mapPos - pos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;
  for (int iterations = 0; iterations < VOXEL_BRICK_ITERATIONS; iterations++) {
    if (hit > 0.0 || voxel_get(volume, ivec3(mapPos))) {
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  pos = floor(mapPos);
  normal = mask;
  return hit;
}
