#version 430 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

uniform mat4 VP;

layout (std430, binding=9) buffer pointsBuffer {
   vec4 points[];
};

void main() {
  gl_Position = VP * points[gl_InstanceID];
}
