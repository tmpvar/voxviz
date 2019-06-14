#version 430 core
#extension GL_NV_gpu_shader5: enable
#extension GL_ARB_bindless_texture : require

#include "voxel-space.glsl"

layout (std430) buffer inColorBuffer {
  vec4 in_color[];
};
layout (std430) buffer inTerminationBuffer {
  RayTermination in_termination[];
};
uniform uvec2 resolution;

in vec2 uv;

layout(location = 0) out vec4 outColor;

void main() {
  uvec2 color_pos = uvec2(gl_FragCoord.xy);
  //    (0.5 + gl_FragCoord.xy * 0.5) *  vec2(resolution)
  // );
  uint color_idx = color_pos.x + color_pos.y * resolution.x;
  outColor = in_color[color_idx];
  //outColor = in_termination[color_idx].color;
  //outColor = vec4(gl_FragCoord.xy / vec2(resolution), 0.0, 0.0);
}
