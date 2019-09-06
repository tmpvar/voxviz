#version 430 core
#extension GL_NV_gpu_shader5: enable

out vec4 outColor;
uniform uvec2 resolution;

layout (std430) buffer colorBuffer {
  vec4 colors[];
};

void main() {
  uint color_idx = (
    uint(gl_FragCoord.x) +
    uint(gl_FragCoord.y) * resolution.x
  );
  outColor = colors[color_idx];
}
