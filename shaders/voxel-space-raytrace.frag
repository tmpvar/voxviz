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

uniform sampler3D worldSpaceVoxelTexture;

#define ITERATIONS 256

#include "voxel-space-mips.glsl"


vec3 compute_ray_dir(vec2 uv, mat4 inv) {
  vec4 far = inv * vec4(uv.x, uv.y, 1.0, 1.0);
  far /= far.w;
  vec4 near = inv * vec4(uv.x, uv.y, 0.5, 1.0);
  near /= near.w;

  return normalize(far.xyz - near.xyz);
}

float read(vec3 pos, float mip) {
  return textureLod(
    worldSpaceVoxelTexture,
    // sample from the voxel center to avoid glitches in debug rendering
    pos/vec3(volumeSlabDims),
    mip
  ).x;
}
#define VOXEL_SIZE 1

bool oob(vec3 p) {
  if (
    any(lessThan(p, vec3(0.0))) ||
    any(greaterThanEqual(p, vec3(volumeSlabDims)))
  ) {
    return true;
  }
  return false;
}

vec3 scaleAndBias(const vec3 p) { return 0.5f * p + vec3(0.5f); }
bool isInsideCube(const vec3 p, float e) { return abs(p.x) < 1 + e && abs(p.y) < 1 + e && abs(p.z) < 1 + e; }

#include "voxel-space/trace.glsl"

// https://github.com/Friduric/voxel-cone-tracing/blob/master/Shaders/Voxel%20Cone%20Tracing/voxel_cone_tracing.frag
float trace_Friduric(vec3 from, vec3 direction, float targetDistance, inout float dist){
	float acc = 0;

	dist = 3 * VOXEL_SIZE;
	// I'm using a pretty big margin here since I use an emissive light ball with a pretty big radius in my demo scenes.
	const float STOP = targetDistance - 16 * VOXEL_SIZE;

	while(dist < STOP && acc < 1){
		vec3 c = from + dist * direction;
		if(oob(c)) {
       break;
    }
		c = scaleAndBias(c);
		float l = pow(dist, 2); // Experimenting with inverse square falloff for shadows.
		float s1 = 0.062 * textureLod(worldSpaceVoxelTexture, c, 1 + 0.75 * l).r;
		float s2 = 0.135 * textureLod(worldSpaceVoxelTexture, c, 4.5 * l).r;
		float s = s1 + s2;
		acc += (1 - acc) * s;
		dist += 0.9 * VOXEL_SIZE * (1 + 0.05 * l);
	}
	return 1 - pow(smoothstep(0, 1, acc * 1.4), 1.0 / 1.4);
}

// https://github.com/armory3d/armory_ci/blob/master/test1/build_untitled/compiled/Shaders/std/conetrace.glsl
#define voxelgiOffset 1.0
#define voxelgiResolution .0
vec4 trace_armory(vec3 origin, vec3 dir, const float aperture, const float maxDist) {
	vec4 sampleCol = vec4(0.0);
	float dist = 0.04 * voxelgiOffset;
	float diam = dist * aperture;
	vec3 samplePos;
	// Step until alpha >= 1 or out of bounds
	while (sampleCol.a < 1.0 && dist < maxDist) {
		samplePos = dir * dist + origin;
    if (!isInsideCube(samplePos, 0)) {
      return sampleCol;
    }

		// Choose mip level based on the diameter of the cone
		float mip = max(log2(diam * voxelgiResolution.x), 0);
		// vec4 mipSample = sampleVoxel(samplePos, dir, indices, mip);
		vec4 mipSample = textureLod(worldSpaceVoxelTexture, samplePos * 0.5 + 0.5, mip);
		// Blend mip sample with current sample color
		sampleCol += (1 - sampleCol.a) * mipSample;
		dist += max(diam / 2, VOXEL_SIZE); // Step size
		diam = dist * aperture;
	}
	return sampleCol;
}

vec4 trace_mine(vec3 pos, vec3 dir) {
  float sampleCol = 0.0;
  float dist = 0.0;
  float aperture = 2.0 / tan(3.1459 / 2.0);
  float diam = dist * aperture;
  vec3 samplePos = dir * dist + pos;
  float maxDist = 512;
  float occ = 0.0;
  while (occ < 1.0 && dist < maxDist) {
    if (oob(samplePos)) {
      return vec4(occ);
    }

    float mip = max(log2(diam/2), 0);

    float s0 = 0.520 * read(samplePos, diam);
    float s1 = 0.022 * read(samplePos, 1 + 0.95 * diam);
    float s2 = 0.035 * read(samplePos, 1.5 *  diam);
    float mipSample = (s1 + s2 + s0);

    occ += (1.0 - occ) * mipSample;// / (2.0 + 0.3 * diam);
    sampleCol += (1 - sampleCol) * mipSample;
    dist += max(diam / 2, VOXEL_SIZE);
    //diam = dist * aperture;
    diam = max(1, 2.0 * aperture * dist);
    samplePos = dir * dist + pos;
  }

  return vec4(occ);
}

void main2() {
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

  //outColor = vec4(1.0 - found_distance/1000.0);

  vec3 d = in_ray_dir;
  vec3 pos = eye + d * found_distance;

  // outColor = vec4(trace_Friduric(pos, d, 256.0, found_distance));
  // outColor = trace_armory(pos / vec3(volumeSlabDims), d,  0.5, 1.0);
   outColor = 1.0 - trace_mine(pos, d);
  //outColor = vec4(traceMine(pos, pos+d));

  //acc += (light_col * 1.0/sampleCol);//* (1.0 - sampleCol);
  // outColor = vec4(
  //   occ,
  //   0.0,
  //   0.0, //1.0 - found_distance,
  //   1.0
  // );
  // acc += vec3(1.0 - occ);
 //  return 1;//1.0 - occ;
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
  int mip = 0;
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

    // if (voxel_mip_get(p, mip, palette_idx)) {
    if (read(p, mip) > 0.0) {
      if (mip == minMip) {
        //c = palette_color(palette_idx);

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
    else if (mip < maxMip && read(p, min(mip + 1, maxMip)) == 0.0) {
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


  if (miss || debug == 1.0) {
    c = hsl(vec3(0.7 - (float(i)/float(ITERATIONS) * 0.9), 0.9, 0.5));
    outColor = vec4(c, 1.0);
  } else {
    outColor = vec4(normalize(found_normal) * 0.5 + 0.5, 1.0);
    //outColor = vec4(p/vec3(volumeSlabDims), 1.0);
    //outColor = vec4(read(p, minMip));
  }
}
