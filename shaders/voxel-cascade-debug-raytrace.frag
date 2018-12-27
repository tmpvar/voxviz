#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"
#include "hsl2rgb.glsl"

out vec4 outColor;

in vec3 ray_dir;

uniform vec3 center;

layout (std430, binding=0) buffer cascade_index
{
  Level levels[TOTAL_VOXEL_CASCADE_LEVELS];
};

layout (std430, binding=1) buffer cascade_slab
{
  SlabEntry entry[];
};

#define ITERATIONS BRICK_DIAMETER * 2

bool voxel_cascade_get(uint levelIdx, ivec3 pos) {
  if (levelIdx >= TOTAL_VOXEL_CASCADE_LEVELS) {
    return false;
  }

  Level level = levels[levelIdx];

  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return false;
  }

  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  Cell cell = level.cells[idx];
  return cell.state > 0;
}


float march_cascade(in vec3 cascadeCenter, vec3 rayDir, in int levelIdx, out vec3 pos, out vec3 normal, out float iterations) {
  vec3 mapPos = vec3(0.0);
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = ((sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
  float hit = 0.0;
  for (iterations = 0; iterations < ITERATIONS; iterations++) {
    if (voxel_cascade_get(levelIdx, ivec3(floor(mapPos) + BRICK_RADIUS))) {
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }
  
  float cellSize = float(1 << (levelIdx + 1));
  pos = cascadeCenter + floor(mapPos) * cellSize;
  normal = mask;
  return hit;
}

void levelColor(in out vec3 color, float hit, int levelIdx, vec3 normal) {
  if (hit > 0.0) {
    vec3 rgb = hsl2rgb(float(levelIdx)/float(TOTAL_VOXEL_CASCADE_LEVELS), 0.9, .4);
    if (normal.x > 0) {
      rgb *= vec3(0.5);
    }
    if (normal.y > 0) {
      rgb *= vec3(1.0);
    }
    if (normal.z > 0) {
      rgb *= vec3(0.75);
    }
    color = rgb;
  }
}

void main() {
  vec3 pos;
  vec3 normal;
  float iterations;
  float dist = 0.9999;
  float newDist = 0.0;
  vec3 color = abs(normalize(ray_dir));
  float hit = 0.0;

  int levelIdx = 7;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;

  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;
  
  // cascade level
  hit = march_cascade(center, ray_dir, levelIdx, pos, normal, iterations);
  dist = min(distance(pos, center) / MAX_DISTANCE, dist);
  levelColor(color, hit, levelIdx, normal);
  levelIdx--;
  
  gl_FragDepth = dist;
  outColor = vec4(color, 1.0);


}