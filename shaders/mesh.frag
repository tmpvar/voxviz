#version 450 core
#extension GL_NV_gpu_shader5: enable

out vec4 outColor;
in vec3 vert_pos;
uniform uint totalTriangles;
uniform uvec2 resolution;

#include "hsl.glsl"
#include "../include/core.h"

layout (std430) buffer gbuffer {
  GBufferPixel pixels[];
};

void main() {
  outColor = vec4(
    hsl(vec3(float(gl_PrimitiveID)/float(totalTriangles), 0.9, 0.45)),
    1.0
  );

  uint pixel_idx = (
    uint(gl_FragCoord.x) +
    uint(gl_FragCoord.y) * resolution.x
  );

  GBufferPixel p;
  p.position = vec4(vert_pos, 0.0);
  pixels[pixel_idx] = p;
}
