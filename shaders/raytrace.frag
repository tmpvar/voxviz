#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require
//#extension GL_EXT_shader_image_load_store : require

#include "voxel.glsl"

in vec3 brickSurfacePos;
flat in vec3 brickTranslation;
flat in uint32_t *volumePointer;
//flat in layout(bindless_sampler) sampler3D volumeSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outPosition;

// main binds

uniform float debug;

uniform vec3 invEye;
uniform int showHeat;
uniform float maxDistance;
uniform vec4 material;
uniform mat4 model;
uniform vec3 eye;
//#define ITERATIONS BRICK_DIAMETER*2
#define ITERATIONS BRICK_DIAMETER*2 + BRICK_RADIUS

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
  hit = 0.0;
  iterations = 0.0;
  for (float i = 0.0; i < ITERATIONS; i++ ) {
    if (hit > 0.0 || voxel_get(volumePointer, ivec3(floor(pos)))) {
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
	float hit = 0.0;
	for (iterations = 0; iterations < ITERATIONS*20; iterations++) {
		if (voxel_get(volumePointer, ivec3(floor(pos)))) {
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

vec3 tx(mat4 m, vec3 v) {
  vec4 tmp = m * vec4(v, 1.0);
  return tmp.xyz / tmp.w;
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

  float hit = march(pos, dir, voxelCenter, normal, iterations);
  //float hit = march_groundtruth(pos, dir, voxelCenter, normal, iterations);
  if (hit < 0.0) {
	discard;
  }
  pos = pos / BRICK_DIAMETER;
  //gl_FragDepth = hit < 0.0 ? 1.0 : distance(rayOrigin + (pos - brickOrigin) / BRICK_DIAMETER, invEye) / maxDistance;
  outColor = mix(material, vec4(1.0, 0.0, 0.0, 1.0), debug); //vec4(brickOrigin / float(BRICK_DIAMETER), 1.0), hit < 0.0);
  //outPosition = rayOrigin / maxDistance;


  gl_FragDepth = mix(
	1.0,
	distance(tx(model, brickTranslation + pos), eye) / maxDistance,
	hit
  );

  //outColor = vec4(pos, 1.0);
  //outColor = vec4(0.5 * dir + 0.5, 1.0);
  outColor = vec4(normal, 1.0);
  outColor = mix(vec4(normal, 1.0), vec4(1.0, 0.0, 0.0, 1.0), debug);
  //outColor = vec4(brickSurfacePos, 1.0);
  //outColor = material;
}
