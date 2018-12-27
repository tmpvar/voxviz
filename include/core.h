#ifndef __CORE_H
#define __CORE_H

#define BRICK_DIAMETER 16
#define BRICK_RADIUS BRICK_DIAMETER / 2
#define INV_BRICK_DIAMETER 1.0 / float(BRICK_DIAMETER)
#define BRICK_VOXEL_COUNT BRICK_DIAMETER*BRICK_DIAMETER*BRICK_DIAMETER

#define VOXEL_WORD_BITS 32
#define BRICK_VOXEL_BYTES BRICK_VOXEL_COUNT / 8
#define BRICK_VOXEL_WORDS BRICK_VOXEL_COUNT / VOXEL_WORD_BITS
#define MAX_DISTANCE 10000

#define TOTAL_COMMAND_QUEUES 16


#define TOTAL_VOXEL_CASCADE_LEVELS 8

#define RENDER_STATIC 1
//#define RENDER_DYNAMIC 1

//#define FULLSCREEN

#endif
