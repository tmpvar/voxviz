#version 430 core
#extension GL_NV_gpu_shader5: require

#include "../include/core.h"
#include "voxel-cascade.glsl"
#include "ray-aabb.glsl"
#include "hsl2rgb.glsl"

out vec4 outColor;

in vec3 ray_dir;

uniform vec3 center;
uniform vec3 eye;
uniform Cell *uniformGridIndex;
uniform uvec3 dims;

layout (std430, binding=1) buffer uniformGridSlab
{
  SlabEntry entry[];
};

#define ITERATIONS 256

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

bool march(in out vec3 pos, vec3 rayDir, out vec3 center, out vec3 normal, out Cell cell) {
  vec3 mapPos = floor(pos += vec3(dims) * 0.5);
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = ((sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
  bool hit = false;
  vec3 prevPos = pos;
  for (int iterations = 0; iterations < ITERATIONS; iterations++) {
    hit = uniform_grid_get(
      uniformGridIndex,
      ivec3(floor(mapPos)),
      cell
    );

    if (hit) {
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  pos += floor(mapPos);
  normal = mask;
  return hit;
}

void main() {
  vec3 color = vec3(0.0);//abs(ray_dir);
  vec3 pos = vec3(0.0);
  vec3 normal;
  

  vec3 found_center, found_normal;
  Cell found_cell;
  bool hit = march(pos, ray_dir, found_center, found_normal, found_cell);

  if (hit) {
    color = vec3(1.0, 1.0, 1.0);
    vec3 inv_ray_dir = 1.0 / ray_dir;
    // TODO: ray slab against every (up to 512) brick inside of the cell
    const uint max_iterations = found_cell.end - found_cell.start;
    for (uint i=0; i<128; i++) {
      color = vec3(0.0, 0.0, 1.0);
      if (i >= max_iterations) {
        color = vec3(1.0, 0.0, 1.0);
        //color = hsl2rgb(float(i)/(float(max_iterations) + 1.0), 0.9, .4);
        break;
      }
      uint entry_index = i + found_cell.start;
      // TODO: handle arbitrary transforms
      SlabEntry e = entry[entry_index];
      vec3 boxLower = tx(e.transform, vec3(e.brickIndex));
      vec3 boxCenter = tx(e.transform, vec3(e.brickIndex) + vec3(0.5));
      vec3 boxUpper = tx(e.transform, vec3(e.brickIndex) + vec3(1.0));

      //if (ray_aabb(min(boxLower, boxUpper), max(boxLower, boxUpper), eye, inv_ray_dir)) {
      if (test_ray_aabb(boxCenter, abs(boxCenter - boxLower), eye, ray_dir, inv_ray_dir)) {
        color = vec3(0.0, 1.0, 0.0);
        break;
      }
    }

    if (found_normal.x > 0) {
      color *= vec3(0.5);
    }
    if (found_normal.y > 0) {
      color *= vec3(1.0);
    }
    if (found_normal.z > 0) {
      color *= vec3(0.75);
    }
  }

  float dist = distance(pos, center) / MAX_DISTANCE;

  //gl_FragDepth = dist;
  
  outColor = vec4(color, 1.0);
  //outColor = vec4(dist);
}