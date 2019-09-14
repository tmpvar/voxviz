#version 450 core
#extension GL_NV_gpu_shader5: require

#include "../ray-aabb.glsl"
#include "../voxel-space.glsl"
#include "../hsl.glsl"
#include "../voxel-space-mips.glsl"

layout(location = 0) out vec4 outColor;

layout (std430) buffer lightIndexBuffer {
  uint current_light_index;
};

struct Light {
  vec4 position;
  vec4 color;
};

layout (std430) buffer lightBuffer {
  Light lights[];
};

layout (std430) buffer blueNoiseBuffer {
  vec4 blueNoise[];
};

uniform uint time;

#include "../cdir.glsl"

uniform uvec2 resolution;

uniform sampler2D gBufferPosition;
uniform sampler2D gBufferColor;
uniform sampler2D gBufferDepth;
uniform sampler2D gBufferNormal;

uniform mat4 VP;

#define ITERATIONS 64

uint traceLight(vec3 pos, Light light, inout vec3 acc) {
  vec3 light_pos = light.position.xyz;
  vec3 light_col = light.color.rgb;

  uint8_t palette_idx;

  vec3 d = normalize(light_pos - pos);
  float found_distance = 1.9;
  vec3 id = 1.0 / d;
  vec3 p = pos + d * found_distance;
  // vec3 prevP = pos;// - d * 2.0;

  float dt = 1.0 / length(volumeSlabDims);
  float t = found_distance;

  int minMip = 0;
  int maxMip = 0;
  int mip = minMip;
  float mipSize = (float(1<<(mip)));
  float invMipSize = 1.0 / mipSize;
  // vec3 mask;
  // bool miss = true;

  for (int i=0; i<ITERATIONS; i++) {
    float distance_to_light = distance(p, light_pos);
    if (distance_to_light < 3.0) {
      acc += light_col * distance_to_light / 3.0;// * (1.0 - float(mipSize)/float(1<<MAX_MIPS));
      return 1;
    }

    if (voxel_mip_get(p, mip, palette_idx)) {
      return 0;
    }

    vec3 deltas = (step(0.0, d) - fract(p * invMipSize)) * id;
    dt = max(mincomp(deltas), .007) * mipSize;
    t += dt * 2.0;
    p = pos + d * t;
  }
  return 0;
}

void main() {
  ivec2 tx = ivec2(gl_FragCoord.xy);
  // if (tx.x % 2 > 0 || tx.y % 2 > 0) {
  //   discard;
  // }
  float depth = texelFetch(gBufferDepth, tx, 0).x;
  vec3 pos = texelFetch(gBufferPosition, tx, 0).xyz * vec3(volumeSlabDims);
  //vec4 color = texelFetch(gBufferColor, tx, 0);

  if (depth >= 1.0) {
    discard;
  }

  vec3 acc = vec3(0.0);
  uint total_lights = min(current_light_index, 64);
  uint contribs = 1;
  for (uint i=0; i<total_lights; i++) {
    contribs += traceLight(pos, lights[i], acc);
  }

  acc /= float(contribs);

  // outColor = vec4(pos, 0);// * vec4(acc, 1.0);
  // outColor = vec4(depth / 10.0);

  // outColor = texelFetch(
  //   gBufferPosition,
  //   ivec2(gl_FragCoord.xy),
  //   0
  // );
  //
  //outColor = color * vec4(acc, 1.0);
  outColor = vec4(acc, 1.0);
}