#version 450 core
#extension GL_NV_gpu_shader5: enable

layout (location=0) out vec3 gPosition;
layout (location=1) out vec4 gColor;

//out vec4 outColor;
in vec3 vert_pos;
uniform uint totalTriangles;
uniform uvec2 resolution;
uniform vec3 volumeSlabDims;
uniform vec3 eye;

#include "hsl.glsl"
#include "../include/core.h"

void main() {
  gColor = vec4(
    hsl(vec3(float(gl_PrimitiveID)/float(totalTriangles), 0.9, 0.45)),
    1.0
  );

  uint pixel_idx = (
    uint(gl_FragCoord.x) +
    uint(gl_FragCoord.y) * resolution.x
  );

  gPosition = vert_pos / volumeSlabDims;

  gl_FragDepth = distance(eye, vert_pos) / 1000.0;
}
