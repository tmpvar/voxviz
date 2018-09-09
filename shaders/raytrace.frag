#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require
//#extension GL_EXT_shader_image_load_store : require

#include "../include/core.h"

in vec3 rayOrigin;
in vec3 brickOrigin;

flat in float *volumePointer;
//flat in layout(bindless_sampler) sampler3D volumeSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outPosition;

// main binds

uniform float debug;
uniform vec3 eye;
uniform vec3 invEye;
uniform int showHeat;
uniform float maxDistance;
uniform vec4 material;
#define ITERATIONS BRICK_DIAMETER*3

float voxel(vec3 gridPos) {
  uvec3 pos = uvec3(round(gridPos));
  uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  bool oob = any(lessThan(pos, vec3(0.0))) || any(greaterThanEqual(pos, vec3(BRICK_DIAMETER)));
  return oob ? -1.0 : float(volumePointer[idx]);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 0.6666666666, 0.33333333333, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 heat(float amount, float total) {
  float p = (amount / total);
  vec3 hsv = vec3(0.66 - p, 1.0, .75);
  return vec4(hsv2rgb(hsv), 1.0);
}

float march(in out vec3 pos, vec3 dir, out vec3 center, out vec3 normal, out float iterations) {
  // grid space
  vec3 origin = pos;
  vec3 grid = floor(pos);
  vec3 grid_step = sign( dir );
  vec3 corner = max( grid_step, vec3( 0.0 ) );
  bvec3 mask;
  float hit = 0.0;
  // ray space
  vec3 inv = vec3( 1.0 ) / dir;
  vec3 ratio = ( grid + corner - pos ) * inv;
  vec3 ratio_step = grid_step * inv;

  // dda
  hit = -1.0;
  iterations = 0.0;
  for (float i = 0.0; i < ITERATIONS; i++ ) {
    if (hit > 0.0 || voxel( grid ) > 0.0) {
      hit = 1.0;
	  continue;
    }
    iterations++;

    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    grid += ivec3(grid_step) * ivec3(mask);
    pos += grid_step * ivec3(mask);
    ratio += ratio_step * vec3(mask);
  }
  
  center = floor(pos) + vec3( 0.5 );
  vec3 d = abs(center - pos);

  normal = iterations == 0.0 ? vec3(greaterThan(d.xyz, max(d.yzx, d.zxy))) : vec3(mask);
  return hit;
}

void main() {
  // TODO: handle the case where the camera is inside of a volume
  vec3 eyeToPlane = rayOrigin - invEye;
  vec3 dir = normalize(eyeToPlane);
  vec3 normal;
  vec3 voxelCenter;
  float iterations;
  
  // move the location to positive space to better align with the underlying grid
  vec3 pos = brickOrigin;

  float hit = march(pos, dir, voxelCenter, normal, iterations);
 
  gl_FragDepth = hit < 0.0 ? 1.0 : distance(rayOrigin + (pos - brickOrigin), invEye) / maxDistance;
  outColor = mix(material, vec4(brickOrigin / float(BRICK_DIAMETER), 1.0), hit < 0.0);
  outPosition = rayOrigin / maxDistance;

}

void main1() {
	outColor = vec4(1.0);
}
