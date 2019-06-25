#version 450 core
#extension GL_NV_gpu_shader5: require

#include "voxel.glsl"

in vec3 brickSurfacePos;
flat in vec3 brickTranslation;
flat in uint32_t brick_index;

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

float march(in out vec3 pos, vec3 rayDir, out vec3 center, out vec3 normal, out float iterations) {
  vec3 del = fract(pos) - rayDir;
  pos -= rayDir * 0.1;//step(del.xyz, del.yzx) * step(del.xyz, del.zxy) * 0.1;
  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);

  vec3 sideDist = (sign(rayDir) * (mapPos - pos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;

  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;
  vec3 prevPos = pos;
  for (int iterations = 0; iterations < ITERATIONS && hit == 0.0; iterations++) {
    // TODO: this used to work in the function call but now if we bail we'll stop iterating
    //       which is a bad thing.
    if (all(greaterThanEqual(mapPos, vec3(0))) && all(lessThan(mapPos, vec3(BRICK_DIAMETER)))) {
      if (voxel_get(brick_index, mapPos)) {
        hit = 1.0;
        break;
      }
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  pos = floor(mapPos) + 0.5;

  normal = mask;
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
  vec3 tpos = tx(model, brickTranslation + pos);
  outPosition = tpos / maxDistance;

  gl_FragDepth = mix(
  	1.0,
  	distance(tpos, eye) / maxDistance,
  	hit
  );

  //outColor = vec4(pos, 1.0);
  //outColor = vec4(0.5 * dir + 0.5, 1.0);
  outColor = vec4(normal, 1.0);
  outColor = mix(vec4(normal, 1.0), vec4(1.0, 0.0, 0.0, 1.0), debug);
  //outColor = vec4(brickSurfacePos, 1.0);
  //outColor = material;
}
