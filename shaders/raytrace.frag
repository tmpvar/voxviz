#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require
//#extension GL_EXT_shader_image_load_store : require

#include "../include/core.h"

in vec3 brickSurfacePos; 
flat in vec3 brickTranslation; 
flat in float *volumePointer;
//flat in layout(bindless_sampler) sampler3D volumeSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outPosition;

// main binds

uniform float debug;

uniform vec3 invEye;
uniform int showHeat;
uniform float maxDistance;
uniform vec4 material;
//#define ITERATIONS BRICK_DIAMETER*2
#define ITERATIONS BRICK_DIAMETER*2 + BRICK_RADIUS

float voxel(ivec3 pos) {
	if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
		return -1;
	}

	uint idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
	return volumePointer[idx];
}

float march1(in out vec3 pos, vec3 dir, out vec3 center, out vec3 normal, out float iterations) {
  // grid space
  vec3 origin = pos;
  //vec3 grid = vec3(floor(pos));
  vec3 grid_step = sign(dir);

  bvec3 mask;
  float hit = -1.0;
  // ray space
  vec3 inv = vec3( 1.0 ) / dir;
  vec3 ratio = inv;
  //vec3 ratio = (1.0-fract(pos)) * inv;
  vec3 ratio_step = grid_step * inv;

  // dda
  iterations = 0.0;
  for (float i = 0.0; i < ITERATIONS; i++ ) {
	float v = voxel(ivec3(pos));
    if (hit > 0.0 || v > 0.0) {
      hit = 1.0;
	  break;
    }
    iterations++;

    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    //grid += uvec3(grid_step) * uvec3(mask);
    pos += grid_step * vec3(mask);
    ratio += ratio_step * vec3(mask);
  }
  
  center = floor(pos) + vec3( 0.5 );
  vec3 d = abs(center - pos);
  normal = iterations == 0.0 ? vec3(greaterThan(d.xyz, max(d.yzx, d.zxy))) : vec3(mask);
  return hit;
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
    if (hit > 0.0 || voxel( ivec3(floor(pos)) ) > 0.0) {
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

float march_groundtruth(in out vec3 pos, vec3 dir, out vec3 center, out vec3 normal, out float iterations) {
	vec3 invDir = 1.0 / dir;
	float hit = -1.0;
	for (iterations = 0; iterations < ITERATIONS*20; iterations++) {
		float v = voxel(ivec3(floor(pos)));
		if (v > 0.0) {
			hit = 1.0;
			break;
		}
		pos += dir / 5;
	}

	center = floor(pos) + vec3( 0.5 );
	vec3 d = sign(fract(pos));

	normal = vec3(lessThanEqual(d.xyz, min(d.yzx, d.zxy)));
	

	return hit;
}

void main() {

  // TODO: handle the case where the camera is inside of a brick
  vec3 eyeToPlane = (brickSurfacePos + brickTranslation) - invEye;
  vec3 dir = normalize(eyeToPlane);
  vec3 normal;
  vec3 voxelCenter;
  float iterations;
  
  // move the location to positive space to better align with the underlying grid
  vec3 pos = brickSurfacePos * BRICK_DIAMETER;

  //float hit = march(pos, dir, voxelCenter, normal, iterations);
  float hit = march(pos, dir, voxelCenter, normal, iterations);
  
  pos = pos / BRICK_DIAMETER;
  //gl_FragDepth = hit < 0.0 ? 1.0 : distance(rayOrigin + (pos - brickOrigin) / BRICK_DIAMETER, invEye) / maxDistance;
  outColor = material; //vec4(brickOrigin / float(BRICK_DIAMETER), 1.0), hit < 0.0);
  //outPosition = rayOrigin / maxDistance;
  gl_FragDepth = hit > 0.0 ? distance(brickTranslation + pos, invEye) / maxDistance : 1.0;
  outColor = vec4(pos, 1.0);
  //outColor = vec4(0.5 * dir + 0.5, 1.0);
  //outColor = vec4(normal, 1.0);
  //outColor = vec4(brickSurfacePos, 1.0);
  //outColor = material;
}

void main1() {
	outColor = vec4(
		vec3(voxel(ivec3(brickSurfacePos * BRICK_DIAMETER - 0.1))),
		1.0
	);

	//outColor = vec4(voxel(uvec3(ceil(brickSurfacePos * BRICK_DIAMETER - 1))));
//	if (edgeVoxel < 0.0) {
//		discard;
//	} else {
//		outColor = vec4(brickSurfacePos, 1.0);
//	}
	//outColor = vec4(floor(brickSurfacePos * BRICK_DIAMETER) / BRICK_DIAMETER, 1.0);
}
