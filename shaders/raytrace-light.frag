#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "voxel.glsl"

in vec3 rayOrigin;
in vec3 center;
in vec3 brickSurfacePos;
flat in vec3 brickTranslation;

flat in uint32_t *volumePointer;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
//layout(location = 2) out float outDepth;

uniform sampler3D volume;
uniform uvec3 dims;
uniform vec3 eye;
uniform vec3 invEye;
uniform float maxDistance;
uniform mat4 model;
uniform vec4 material;
uniform float debug;

vec3 tx(mat4 m, vec3 v) {
  vec4 tmp = m * vec4(v, 1.0);
  return tmp.xyz / tmp.w;
}

#define ITERATIONS BRICK_DIAMETER*2 + BRICK_RADIUS

float march(in out vec3 pos, vec3 rayDir, out vec3 center, out vec3 normal, out float iterations) {
  pos -= rayDir * 4.0;
  vec3 mapPos = vec3(floor(pos));
  vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);
  vec3 rayStep = sign(rayDir);
  vec3 sideDist = (sign(rayDir) * (mapPos - pos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;
  vec3 mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);

  float hit = 0.0;
  vec3 prevPos = pos;
  for (int iterations = 0; iterations < ITERATIONS; iterations++) {
    if (hit > 0.0 || voxel_get(volumePointer, ivec3(mapPos))) {
      hit = 1.0;
      break;
    }

    mask = step(sideDist.xyz, sideDist.yzx) * step(sideDist.xyz, sideDist.zxy);
    sideDist += mask * deltaDist;
    mapPos += mask * rayStep;
  }

  pos = floor(mapPos) + 0.5;
  normal = mask;
  return hit;
}

void main() {
  // TODO: handle the case where the camera is inside of a brick
  vec3 eyeToPlane = (brickSurfacePos + brickTranslation) - invEye;
  vec3 dir = normalize(eyeToPlane);
  vec3 normal;
  vec3 voxelCenter;
  float iterations;
  vec3 found_normal;
  float found_iterations;
  vec3 found_center;

  // move the location to positive space to better align with the underlying grid
  vec3 pos = brickSurfacePos * BRICK_DIAMETER;


  float hit = march(pos, dir, found_center, found_normal, found_iterations);
  vec3 tpos = tx(model, brickTranslation + brickSurfacePos);
  gl_FragDepth = distance(pos, eye) / maxDistance;
  gl_FragDepth = distance(tpos, eye) / maxDistance;
  outColor = vec4(normalize(pos), 1.0);
  outPosition = vec4(tpos, 1.0);
}
