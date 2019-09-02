#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "ray-aabb.glsl"
#include "dda-cursor.glsl"
#include "palette.glsl"
#include "cosine-direction.glsl"
#include "voxel-space.glsl"
#include "hsl.glsl"

layout(location = 0) out vec4 outColor;

// main binds
uniform float debug;
uniform int showHeat;
uniform float maxDistance;
uniform vec3 eye;
uniform uint time;
uniform mat4 VP;
uniform uvec3 lightPos;
uniform vec3 lightColor;
uniform uvec2 resolution;

#define ITERATIONS 512

#include "voxel-space-mips.glsl"


vec3 compute_ray_dir(vec2 uv, mat4 inv) {
  vec4 far = inv * vec4(uv.x, uv.y, 1.0, 1.0);
  far /= far.w;
  vec4 near = inv * vec4(uv.x, uv.y, 0.5, 1.0);
  near /= near.w;

  return normalize(far.xyz - near.xyz);
}

void main() {

  vec2 uv = vec2(gl_FragCoord.xy) / vec2(resolution) * 2.0 - 1.0;
  vec3 in_ray_dir = compute_ray_dir(uv, inverse(VP));

  float found_distance;
  vec3 found_normal = vec3(0.0);
  vec3 hd = volumeSlabDims * 0.5;

  bool hit;
  if (all(greaterThanEqual(eye, vec3(0.0))) && all(lessThan(eye, volumeSlabDims))) {
    hit = true;//pos = eye + in_ray_dir * 0.001;
    found_distance = 0.0;
  } else {
    hit = ourIntersectBoxCommon(
      hd,
      hd,
      eye,
      in_ray_dir,
      1.0 / in_ray_dir,
      found_distance,
      found_normal
    );
    found_distance += 0.1;
  }
  if (!hit) {
    return;
  }

  outColor = vec4(0.2);


  uint8_t palette_idx;
  vec3 c = vec3(0.0);
  int i;

  vec3 d = in_ray_dir;
  vec3 id = 1.0 / d;
  vec3 p = eye + d * found_distance;
  vec3 prevP = eye;// - d * 2.0;

  float dt = 1.0 / length(volumeSlabDims);
  float t = found_distance;
  float opacityDiv = 1.0 / 8.0;

  int minMip = 0;
  int maxMip = MAX_MIPS;
  int mip = maxMip;
  float mipSize = (float(1<<(mip)));
  float invMipSize = 1.0 / mipSize;
  vec3 mask;
  bool miss = true;
  for (i=0; i<ITERATIONS; i++) {
    if (
      any(lessThan(p, vec3(0.0))) ||
      any(greaterThanEqual(p, vec3(volumeSlabDims)))
    ) {
      break;
    }

    if (voxel_mip_get(p, mip, palette_idx)) {
      if (mip == minMip) {
        c = palette_color(palette_idx);

        // if the ray have moved since we intersected the outer bounding box then
        // we should recompute a normal.
        if (found_distance != t) {
          found_normal = -sign(d) *
                          step(prevP.xyz, prevP.yzx) *
                          step(prevP.xyz, prevP.zxy);
        }

        miss = false;
        break;
      }

      mip = max(mip - 1, minMip);
      t -= dt * 0.00125;

      mipSize = (float(1<<(mip)));
      invMipSize = 1.0 / mipSize;
      continue;
    }
    else if (mip < maxMip && !voxel_mip_get(p, min(mip + 1, maxMip), palette_idx)) {
      mip = min(maxMip, mip+1);
      mipSize = (float(1<<(mip)));
      invMipSize = 1.0 / mipSize;
      t += dt * 0.00125;
    }

    vec3 deltas = (step(0.0, d) - fract(p * invMipSize)) * id;
    dt = max(mincomp(deltas), 0.0001) * mipSize;
    t += dt;
    // FIXME: we get ~1ms back if this is enabled, but lose the ability to
    //        compute good face normals. There is likely a better mechanism
    //        for increasing marching speed at further distances (cone marching?)
    //t += max(0.1,t/1024);
    prevP = deltas;
    p = eye + d * t;
  }

    if (true || debug == 1.0) {
      c = hsl(vec3(0.7 - (float(i)/float(ITERATIONS) * 0.9), 0.9, 0.5));
    }
    outColor = vec4(c, 1.0);

}
