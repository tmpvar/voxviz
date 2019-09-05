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

uniform uvec2 resolution;

uniform sampler2D gBufferPosition;
uniform sampler2D gBufferColor;
uniform sampler2D gBufferDepth;

uniform vec3 eye;
uniform mat4 VP;

#define ITERATIONS 128

void main() {

  if (gl_FragCoord.x < resolution.x / 2) {
    discard;
  }

  vec3 pos = texelFetch(
    gBufferPosition,
    ivec2(gl_FragCoord.xy),
    0
  ).xyz * float(volumeSlabDims);

  outColor = texelFetch(
    gBufferPosition,
    ivec2(gl_FragCoord.xy),
    0
  );
}
