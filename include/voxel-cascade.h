#pragma once

#include "volume.h"
#include <glm/glm.hpp>
#include <collision/aabb-obb.h>
#include "aabb.h"

struct SlabEntry {
  uint32_t volume;
  glm::ivec3 brickIndex;
  uint64_t brickData;
};

struct GPUCell { 
  uint8_t state; // 0 empty, 0x01 has something
  uint32_t start;
  uint32_t end;
};

struct CPUCell {
  vector<Brick *> bricks;
};

struct CPULevel {
  CPUCell **cells;
};

class VoxelCascade {
  int level_count;

  CPULevel *levels;
  size_t total_cell_bytes = 0;
 
  size_t slab_size = 0;
  size_t slab_pos = 0;
  size_t total_slab_bytes = 0;
  SlabEntry *slab = nullptr;

  GPUCell *gpu_cells;

  glm::vec3 center;

  public:
  VoxelCascade(int level_count) {
    this->level_count = level_count;
    size_t total_level_bytes = sizeof(CPULevel) * this->level_count;
    this->levels = (CPULevel *)malloc(total_level_bytes);

    for (int i = 0; i < level_count; i++) {
      this->levels[i].cells = (CPUCell **)malloc(BRICK_VOXEL_COUNT * sizeof(CPUCell));
      for (int j = 0; j < BRICK_VOXEL_COUNT; j++) {
        this->levels[i].cells[j] = new CPUCell;
      }
    }
    
    // TODO: GPUCell
    size_t gpu_cells_bytes = BRICK_VOXEL_COUNT * level_count * sizeof(GPUCell);
    this->gpu_cells = (GPUCell *)malloc(gpu_cells_bytes);
    memset(this->gpu_cells, 0, gpu_cells_bytes);
  };

  void begin(glm::vec3 eye) {
    if (this->slab_pos > this->slab_size) {
      // realloc the slab
      if (this->slab != nullptr) {
        free(this->slab);
      }

      this->slab_size = (size_t)(ceilf(this->slab_pos / 1024.0f) * 1024);
      printf("increasing slab size to %li\n", this->slab_size);
      this->total_slab_bytes = sizeof(SlabEntry) * this->slab_size;

      this->slab = (SlabEntry *)malloc(total_slab_bytes);
      
    }

    // reset the slab
    this->slab_pos = 0;
    memset(this->slab, 0, this->total_slab_bytes);

    // reset the previous cpu cascade
    for (int i = 0; i < level_count; i++) {
      for (int j = 0; j < BRICK_VOXEL_COUNT; j++) {
        this->levels[i].cells[j]->bricks.clear();
      }
    }

    this->center = eye;
  };

  void addVolume(Volume *volume) {
    glm::mat4 mat = volume->getModelMatrix();
    
    glm::vec3 txVerts[8];
    glm::ivec3 lower(INT_MAX);
    glm::ivec3 upper(INT_MIN);
    glm::ivec3 offset;

    //glm::ivec3 offset = txPoint(mat, brick->index + offset);
    for (auto& it : volume->bricks) {
      Brick *brick = it.second;
      lower.x = INT_MAX;
      lower.y = INT_MAX;
      lower.z = INT_MAX;

      upper.x = INT_MIN;
      upper.y = INT_MIN;
      upper.z = INT_MIN;


      for (uint8_t i = 0; i < 8; i++) {
        offset.x = float((i & 4) >> 2);
        offset.y = float((i & 2) >> 1);
        offset.z = float(i & 1);
        
        txVerts[i] = txPoint(mat, brick->index + offset);
        lower.x = min(txVerts[i].x, lower.x);
        lower.y = min(txVerts[i].y, lower.y);
        lower.z = min(txVerts[i].z, lower.z);
 
        upper.x = max(txVerts[i].x, upper.x);
        upper.y = max(txVerts[i].y, upper.y);
        upper.z = max(txVerts[i].z, upper.z);
      }
     

      for (int level = 0; level < this->level_count; level++) {
        int cellSize = 1 << (level + 1);
        int prevCellSize = 1 << (level);
        int radius = cellSize * BRICK_RADIUS;
        glm::vec3 pos;
        glm::vec3 v3CellSize = glm::vec3(cellSize);

        glm::vec3 level_lower = this->center - glm::vec3(radius);
        glm::vec3 level_upper = this->center + glm::vec3(radius);

        glm::ivec3 start = glm::ivec3(glm::floor(glm::vec3(lower) / float(cellSize)));// * float(cellSize));
        glm::ivec3 end = glm::ivec3(glm::ceil(glm::vec3(upper) / float(cellSize)));// * float(cellSize));

        // TODO: ensure the start/end lies within the current level
        if (!aabb_overlaps_component(level_lower, level_upper, lower, upper)) {
          continue;
        }

        for (int x = start.x; x < end.x; x++) {
          pos.x = x;
          for (int y = start.y; y < end.y; y++) {
            pos.y = y;
            for (int z = start.z; z < end.z; z++) {
              pos.z = z;

              // TODO: test if current cell intersects the obb
              bool isect = collision_aabb_obb(
                pos * v3CellSize,
                pos * v3CellSize + v3CellSize,
                txVerts
              );

              if (isect) {
                this->addBrickToCell(brick, level, pos);
              }
            }
          }
        }
      }
    }
  };

  void addBrickToCell(Brick *brick, int levelIdx, glm::ivec3 pos) {
    int idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
    CPUCell *cell = this->levels[levelIdx].cells[idx];
    cell->bricks.push_back(brick);
  };

  void end() {
    // Push each cell's bricks into contiguous memory while keeping the index to populate
    // the GPUCell's memory.
    
    
    
    for (int i = 0; i < level_count; i++) {
      size_t level_offset = BRICK_VOXEL_COUNT * i;    
      for (int j = 0; j < BRICK_VOXEL_COUNT; j++) {
        CPUCell *cpu_cell = this->levels[i].cells[j];
        size_t brick_count = cpu_cell->bricks.size();
        if (brick_count == 0) {
          continue;
        }

        GPUCell gpu_cell = this->gpu_cells[level_offset + j];
        // Mark this cell as active
        gpu_cell.state = 1;
        gpu_cell.start = this->slab_pos;
        
        for (auto& brick : cpu_cell->bricks) {
          if (this->slab_pos < this->slab_size) {
            SlabEntry entry = this->slab[this->slab_pos];
            entry.brickData = brick->bufferAddress;
            entry.brickIndex = brick->index;
            // TODO: figure out where volumes live on the GPU
            entry.volume = 0;

          }
          this->slab_pos++;
        }
        
        gpu_cell.end = this->slab_pos;
      }
    }

  };
};