#ifndef __CORE_H
#define __CORE_H

// Production toggles to improve perf
//#define DISABLE_DEBUG_GL_TIMED_COMPUTE
//#define DISABLE_GL_ERROR


#define BRICK_DIAMETER 128
#define BRICK_RADIUS BRICK_DIAMETER / 2
#define INV_BRICK_DIAMETER 1.0 / float(BRICK_DIAMETER)
#define BRICK_VOXEL_COUNT BRICK_DIAMETER*BRICK_DIAMETER*BRICK_DIAMETER

#define VOXEL_WORD_BITS 32
#define BRICK_VOXEL_WORDS BRICK_VOXEL_COUNT / VOXEL_WORD_BITS
//#define BRICK_VOXEL_BYTES BRICK_VOXEL_WORDS * 4
#define BRICK_VOXEL_BYTES BRICK_VOXEL_COUNT * 4

#define TOTAL_COMMAND_QUEUES 16

#define RENDER_STATIC 1
#define SHADER_HOTRELOAD 1


#define TAA_HISTORY_LENGTH 16
#define MAX_MIP_LEVELS 7


#ifdef GPU_HOST
  #include <glm/glm.hpp>
  using namespace glm;
#endif


struct Light {
  vec4 position;
  vec4 color;
};

struct RayTermination {
  vec4 position;
  vec4 normal;
  vec4 color;
  vec4 rayDir;
  uvec4 index;
  vec4 light;
};

#endif
