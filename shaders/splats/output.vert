#version 430 core

#extension GL_NV_gpu_shader5: enable

in vec3 position;
in uvec2 resolution;

void main() {
  gl_Position = vec4(position, 1.0);
}
