#ifndef __CORE_H
#define __CORE_H

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

//#define RENDER_DYNAMIC 1

//#define FULLSCREEN
#ifdef GPU_HOST
  #include <glm/glm.hpp>
  #include <stdint.h>
  using namespace glm;

  typedef uint8_t u8;
  typedef uint16_t u16;
  typedef uint32_t u32;
  typedef uint64_t u64;
  typedef float f32;
  typedef double f64;
#endif

#endif
