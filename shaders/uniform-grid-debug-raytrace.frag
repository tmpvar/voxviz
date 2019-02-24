#version 450 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"
#include "voxel.glsl"
#include "ray-aabb.glsl"
#include "hsl2rgb.glsl"

out vec4 outColor;

in vec3 in_ray_dir;

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

layout (std430, binding=2) buffer volumeMaterialSlab
{
  VolumeMaterial volume_material[];
};

#define ITERATIONS 96

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
bool cell_test(in Cell cell, in vec3 ray_origin, in vec3 ray_dir, out float found_distance, out vec3 found_normal, out VolumeMaterial found_material) {
  bool hit = false;
  const uint start = cell.start;
  const uint end = cell.end;
  uint i = 0;
  found_distance = 100000.0;

  for (uint i = start; i<end; i++) {
    SlabEntry e = entry[i];

    // TODO: cache the inverse of this matrix in entry memory.
    mat4 xform = e.invTransform;
    vec3 invOrigin = tx(xform, ray_origin);
    vec3 invDir = normalize(vec4(xform * vec4(ray_dir, 0.0)).xyz);

    vec3 brickCenter = vec3(e.brickIndex) + vec3(0.5);
    float aabb_distance;
    vec3 noop;
    bool aabb_hit = ailaWaldHitAABox(
      brickCenter,
      vec3(0.5),
      invOrigin,
      invDir,
      1.0 / invDir,
      aabb_distance,
      noop
    );

    bool inside = all(greaterThanEqual(invOrigin, vec3(e.brickIndex))) &&
                  all(lessThanEqual(invOrigin, vec3(e.brickIndex) + vec3(1.0)));

    if (inside) {
      aabb_distance = 0.0;
    }

    if (aabb_hit || inside) {
      // TODO: temporary shadow testing

      if (hit && aabb_distance > found_distance) {
        continue;
      }

      vec3 pos = ((invOrigin + invDir * aabb_distance) - vec3(e.brickIndex)) * vec3(BRICK_DIAMETER);
      vec3 out_pos = pos;
      float found_iterations;
      vec3 fn;
      float brick_hit = voxel_march(
        e.brickData,
        out_pos,
        invDir,
        fn,
        found_iterations
      );

      // if we hit a voxel we need to return immediately, otherwise we'll continue
      // traversing the grid cell and potentially miss all of the other bricks which
      // will result in a miss overall.
      if (brick_hit > 0.0) {
        float d = distance(out_pos - pos, invOrigin);
        found_distance = min(found_distance, aabb_distance);
        //if (d < found_distance) {
          //found_distance = min(found_distance, d);
          found_material = volume_material[e.volume_index];
          found_normal = fn;
        //}

        // TODO: instead of returning instantly here, we need to store the closest
        //       brick entry along with its found_distance / surface pos. This way
        //       we don't get weird popping artifacts when bricks that are lower in
        //       index but higher in distance overlap another brick.
        hit = true;
      }
    }
  }
  // always return false here because even though we intersected a brick via
  // ray->aabb we did not hit an actual voxel.
  return hit;
}

void main() {
  vec3 color = vec3(0.0);
  vec3 gridRadius = vec3(dims) / 2.0;
  Cell found_cell;
  vec3 found_normal;
  VolumeMaterial found_material;
  float found_distance;
  vec3 ray_dir = normalize(in_ray_dir);

  DDACursor cursor = dda_cursor_create(eye, center, gridRadius, ray_dir);
  bool hit = false;
  for (uint i = 0; i<ITERATIONS; i++) {
    vec3 no;
    bool stepResult = dda_cursor_step(cursor, no, found_cell);

    if (stepResult) {
      if (cell_test(found_cell, eye, ray_dir, found_distance, found_normal, found_material)) {
        color = found_normal;
        hit = true;

        break;
      }
    }
  }

  //color = vec3(found_distance / 10.0);
  vec3 shadow_ray_dir = normalize(reflect(ray_dir, found_normal));
  vec3 shadow_ray_origin = eye + ray_dir * found_distance + shadow_ray_dir * 0.00001;

  if (hit) {
    color = abs(vec3(0.0, found_distance, 1.0) / 10.0);
    color = found_material.color.rgb;
  }

  if (hit) {
    vec3 shadow_found_normal;
    VolumeMaterial shadow_found_material;
    Cell shadow_found_cell;
    float shadow_found_distance;
    DDACursor shadow_cursor = dda_cursor_create(
      shadow_ray_origin,
      center,
      gridRadius,
      shadow_ray_dir
    );

    for (uint i = 0; i<ITERATIONS; i++) {
      bool shadowStepResult = dda_cursor_step(
        shadow_cursor,
        shadow_found_normal,
        shadow_found_cell
      );

      if (shadowStepResult) {
        // color = vec3(1.0, 0.0, 1.0);

        //color = shadow_found_normal;
        bool shadow_ray_cell_hit = cell_test(
          shadow_found_cell,
          shadow_ray_origin,
          shadow_ray_dir,
          shadow_found_distance,
          shadow_found_normal,
          shadow_found_material
        );

        if (shadow_ray_cell_hit) {
          //color = vec3(1.0, 0.0, 1.0);
          //color = shadow_ray_dir;
          color = vec3(shadow_found_distance / 10.0, found_distance / 10.0, 0);
          color = vec3(1.0);
          color = shadow_found_normal;
          color = shadow_found_material.color.rgb;
          //color = abs(normalize(cross(vec3(1.0, 1.0, 1.0), shadow_found_normal)));
          //color = shadow_found_normal;
          //color = found_normal * 0.25;
          break;
        }
      }
    }
  }
  //gl_FragDepth = found_distance / MAX_DISTANCE;

  outColor = vec4(color, 1.0);
}
