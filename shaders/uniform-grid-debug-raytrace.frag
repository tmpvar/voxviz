#version 450 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"
#include "voxel.glsl"
#include "ray-aabb.glsl"
#include "hsl2rgb.glsl"

out vec4 outColor;

in vec3 ray_dir;

uniform vec3 center;
uniform vec3 eye;
uniform Cell *uniformGridIndex;
uniform uvec3 dims;
uniform float cellSize;
uniform mat4 model;

layout (std430, binding=1) buffer uniformGridSlab
{
  SlabEntry entry[];
};

#define ITERATIONS 16

vec3 tx(mat4 m, vec3 v) {
  vec4 tmp = m * vec4(v, 1.0);
  return tmp.xyz / tmp.w;
}

bool uniform_grid_get(in Cell *uniformGridIndex, in ivec3 pos, out Cell found) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(dims)))) {
    return false;
  }

  uint idx = (pos.x + pos.y * dims.x + pos.z * dims.y * dims.z);
  found = uniformGridIndex[idx];
  return found.state > 0;
}

struct DDACursor {
  vec3 mask;
  vec3 mapPos;
  vec3 rayStep;
  vec3 sideDist;
  vec3 deltaDist;
};

DDACursor dda_cursor_create(
  in const vec3 pos,
  in const vec3 gridCenter,
  in const vec3 gridRadius,
  in const vec3 rayDir
) {
  DDACursor cursor;
  vec3 gp = (pos / cellSize) + gridRadius - gridCenter;
  cursor.mapPos = floor(gp);
  cursor.deltaDist = abs(vec3(length(rayDir)) / rayDir);
  cursor.rayStep = sign(rayDir);
  vec3 p = (cursor.mapPos - gp);
  cursor.sideDist = (
    cursor.rayStep * p + (cursor.rayStep * 0.5) + 0.5
  ) * cursor.deltaDist;
  cursor.mask = step(cursor.sideDist.xyz, cursor.sideDist.yzx) *
                step(cursor.sideDist.xyz, cursor.sideDist.zxy);
  return cursor;
}

bool dda_cursor_step(in out DDACursor cursor, out vec3 normal, out Cell cell) {
  // TODO: allow this to return an int so we can signal oob
  bool hit = uniform_grid_get(
    uniformGridIndex,
    ivec3(floor(cursor.mapPos)),
    cell
  );

  if (hit) {
    normal = cursor.mask;
  }

  vec3 sideDist = cursor.sideDist;
  cursor.mask = step(sideDist.xyz, sideDist.yzx) *
                step(sideDist.xyz, sideDist.zxy);
  cursor.sideDist += cursor.mask * cursor.deltaDist;
  cursor.mapPos += cursor.mask * cursor.rayStep;
  return hit;
}

// TODO: found_distance and found_normal are temporary debugging.
bool cell_test(in Cell cell, out float found_distance, out vec3 found_normal) {
  bool hit = false;
  const uint start = cell.start;
  const uint end = cell.end;
  uint i = 0;

  for (uint i = start; i<end; i++) {
    SlabEntry e = entry[i];

    // TODO: cache the inverse of this matrix in entry memory.
    mat4 xform = inverse(e.transform);
    vec3 invEye = tx(xform, eye);
    vec3 invDir = normalize(tx(xform, ray_dir));

    vec3 brickCenter = vec3(e.brickIndex) + vec3(0.5);
    hit = ailaWaldHitAABox(
      brickCenter,
      vec3(0.5),
      invEye,
      invDir,
      1.0 / invDir,
      found_distance,
      found_normal // TODO: this is probably not needed for our purposes
    );

    if (hit) {
      vec3 pos = ((invEye + invDir * found_distance) - vec3(e.brickIndex)) * vec3(BRICK_DIAMETER);
      vec3 found_center;
      float found_iterations;
      vec3 fn;
      float brick_hit = voxel_march(
        e.brickData,
        pos,
        invDir,
        found_center,
        found_normal,
        found_iterations
      );

      // if we hit a voxel we need to return immediately, otherwise we'll continue
      // traversing the grid cell and potentially miss all of the other bricks which
      // will result in a miss overall.
      if (brick_hit > 0.0) {
        return true;
      }
    }
  }
  return hit;
}

void main() {
  vec3 color = vec3(0.0);
  vec3 gridRadius = vec3(dims) / 2.0;
  Cell found_cell;
  vec3 found_normal;
  float found_distance;
  vec3 r = gridRadius;
  DDACursor cursor = dda_cursor_create(eye, center, r, ray_dir);

  for (uint i = 0; i<ITERATIONS; i++) {
    bool stepResult = dda_cursor_step(cursor, found_normal, found_cell);

    if (stepResult) {
      color = found_normal;
      if (cell_test(found_cell, found_distance, found_normal)) {
        color = found_normal;
        break;
      }
    }
  }

  //gl_FragDepth = found_distance / MAX_DISTANCE;

  outColor = vec4(color, 1.0);
}
