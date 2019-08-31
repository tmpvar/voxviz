#version 430 core

#extension GL_NV_gpu_shader5: enable
#extension GL_ARB_gpu_shader_int64 : enable

#include "../splats.glsl"
#include "../hsl.glsl"

layout (std430) buffer pixelBuffer {
  uint64_t pixels[];
};

uniform uvec2 resolution;
out vec4 outColor;

#define RESET 0xffffffffff000000UL
#define MAX_DEPTH 0xffffffffffUL

vec4 extractColor(uint64_t v){
	uint ucol = uint(v & 0x00FFFFFFUL);

	if(ucol == 0){
		return vec4(0, 0, 0, 255);
	}

	return unpackUnorm4x8(ucol);
  return vec4(
      float(v & 0xFF0000) / float(0xFF0000),
      float(v & 0x00FF00) / float(0x00FF00),
      float(v & 0x0000FF) / float(0x0000FF),
      1.0
  );
}

void main() {
  uint pixel_idx = uint(
      uint(gl_FragCoord.x) +
      uint(gl_FragCoord.y) * resolution.x
  );

  uint64_t v = pixels[pixel_idx];
  outColor = extractColor(v);
  // reset to max depth color 0
  pixels[pixel_idx] = RESET;
}
