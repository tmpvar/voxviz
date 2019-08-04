#version 430 core

#extension GL_NV_gpu_shader5: enable

#include "../splats.glsl"

layout (std430) buffer splatInstanceBuffer {
  Splat splats[];
};

uniform uvec2 res;

out vec4 outColor;
flat in vec4 color;
in vec2 center;
flat in float pointSize;
void main() {

  vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
  if (dot(circCoord, circCoord) > 0.35) {
      discard;
  }
  outColor = color;
}
