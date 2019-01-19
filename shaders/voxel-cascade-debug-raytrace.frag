#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"
#include "hsl2rgb.glsl"

out vec4 outColor;

in vec3 ray_dir;

uniform vec3 center;
uniform Cell *cascade_index;

layout (std430, binding=1) buffer cascade_slab
{
  SlabEntry entry[];
};

#define ITERATIONS BRICK_DIAMETER * 2

float march_cascade(in vec3 cascadeCenter, vec3 rayDir, in uint levelIdx, out vec3 pos, out vec3 normal, out float iterations) {
  vec3 mapPos = vec3(0.0);
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = ((sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
  float hit = 0.0;
  pos = cascadeCenter;
  float cellSize = float(1 << (levelIdx + 1));

  for (iterations = 0; iterations < ITERATIONS; iterations++) {
    Cell found;
    if (voxel_cascade_get(cascade_index, levelIdx, ivec3(floor(mapPos) + BRICK_RADIUS), found)) {
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
    pos += mask * rayStep * cellSize;
  }

  normal = mask;
  return hit;
}

void levelColor(in out vec3 color, float hit, uint levelIdx, vec3 normal) {
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

vec3 getMapPos(in vec3 pos, uint levelIdx) {
  float cellSize = float(1 << (levelIdx + 1));
  return floor(pos/cellSize);
}

struct LevelTraceState {
  float cellSize;
  ivec3 mapPos;
  float pos;
  vec3 deltaDist;
  vec3 rayStep;
  vec3 sideDist;
  vec3 mask;
};

float march(out vec3 pos, out uint levelIdx, out vec3 normal) {
  float hit = 0.0;

  LevelTraceState levelState[TOTAL_VOXEL_CASCADE_LEVELS];

  vec3 mapPos = vec3(0.0);
  vec3 deltaDist = abs(vec3(length(ray_dir)) / ray_dir);
  vec3 rayStep = sign(ray_dir);
  vec3 sideDist = ((sign(ray_dir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  pos = center;
  float cellSize = float(1 << (levelIdx + 1));
  uint iterations;
  for (iterations = 0; iterations < ITERATIONS; iterations++) {
    Cell found;
    if (voxel_cascade_get(cascade_index, levelIdx, ivec3(floor(mapPos) + BRICK_RADIUS), found)) {
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
    pos += mask * rayStep * cellSize;
  }

  return hit;
}

void main() {
  vec3 pos = center;
  vec3 normal;
  float iterations;
  float dist = 0.9999;
  float newDist = 0.0;
  vec3 color = abs(normalize(ray_dir));

  float hit = 0.0;
  VoxelCascadeTraversalState state;
  state.index = uvec3(BRICK_RADIUS);
  state.level = TOTAL_VOXEL_CASCADE_LEVELS - 1;

  vec3 deltaDist = abs(vec3(length(ray_dir)) / ray_dir);
  vec3 rayStep = sign(ray_dir);
  vec3 sideDist = ((sign(ray_dir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  for (uint i = 0; i<100; i++) {
    bool done = voxel_cascade_next(cascade_index, state);


    if (state.level == 0 && !state.empty) {
      color = vec3(0.0, 0.0, 1.0);
      hit = 1.0;
      break;
    }

    if (done) {
       color = vec3(1.0, 0.0, 0.0);
      break;
    }
    
    if (!state.empty) {
      hit = 1.0;
      color = vec3(1.0, 0.0, 1.0);
    }

    if (!state.step) {
      color = vec3(0.0, 1.0, 0.0);
      continue;
    }

    float cellSize = float(1 << (state.level + 1));

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    state.index += ivec3(mask * rayStep);
    pos += mask * rayStep * cellSize;
  }
  

/*  for (uint i=0; i<TOTAL_VOXEL_CASCADE_LEVELS; i++) {
    hit = max(hit, march_cascade(center, ray_dir, i, pos, normal, iterations));
    dist = min(distance(pos, center) / MAX_DISTANCE, dist);
    levelColor(color, hit, i, normal);
    if (hit > 0.0) {
      break;
    }
  }
  */



  if (hit == 0.0) {
    //discard;
    //return;
  }

  dist = distance(pos, center) / MAX_DISTANCE;

  //gl_FragDepth = dist;
  levelColor(color, 1.0, state.level, vec3(1.0, 0.0, 0.0));
  outColor = vec4(color, 1.0);
  //outColor = vec4(dist);
}