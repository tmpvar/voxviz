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

  SSBO *ssbo_index;
  SSBO *ssbo_slab;

  glm::vec3 center;
  Mesh *mesh;
  Program *program;
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

    // Setup the GPU cascade index
    size_t gpu_cells_bytes = BRICK_VOXEL_COUNT * level_count * sizeof(GPUCell);
    this->ssbo_index = new SSBO(gpu_cells_bytes);

    // Setup the GPU cascade slab
    this->ssbo_slab = new SSBO(0);

    this->setupDebugRender();
  };

  void begin(glm::vec3 eye) {
    if (this->slab_pos > this->slab_size) {
      this->slab_size = (size_t)(ceilf(this->slab_pos / 1024.0f) * 1024);
      this->total_slab_bytes = sizeof(SlabEntry) * this->slab_size;
      
      this->ssbo_slab->resize(this->total_slab_bytes);
    }

    // reset the slab
    this->slab_pos = 0;

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
    this->ssbo_slab->resize(this->slab_size * sizeof(SlabEntry));
    SlabEntry *gpu_slab = (SlabEntry *)this->ssbo_slab->beginMap(SSBO::MAP_WRITE_ONLY);
    // TODO: we need to clear this slab or we're going to get ghosting when cells were filled
    //       but are no longer filled.
    GPUCell *gpu_cells = (GPUCell *)this->ssbo_index->beginMap(SSBO::MAP_WRITE_ONLY);
    for (int i = 0; i < level_count; i++) {
      size_t level_offset = BRICK_VOXEL_COUNT * i;    
      for (int j = 0; j < BRICK_VOXEL_COUNT; j++) {
        CPUCell *cpu_cell = this->levels[i].cells[j];
        size_t brick_count = cpu_cell->bricks.size();
        if (brick_count == 0) {
          continue;
        }

        // Mark this cell as active
        GPUCell store;
        store.state = 1;
        store.start = this->slab_pos;
        
        for (auto& brick : cpu_cell->bricks) {
          if (this->slab_pos < this->slab_size) {
            SlabEntry entry;
            entry.brickData = brick->bufferAddress;
            entry.brickIndex = brick->index;
            // TODO: figure out where volumes live on the GPU
            entry.volume = 0;
            if (gpu_slab != nullptr) {
              memcpy(&gpu_slab[this->slab_pos], &entry, sizeof(entry));
            }
          }
          this->slab_pos++;
        }
        
        store.end = this->slab_pos;
        if (gpu_cells != nullptr) {
          memcpy(&gpu_cells[level_offset + j], &store, sizeof(store));
        }
      }
    }
    this->ssbo_slab->endMap();
    this->ssbo_index->endMap();
  };

  void setupDebugRender() {
    this->program = new Program();

    this->program
      ->add(Shaders::get("voxel-cascade-debug.vert"))
      ->add(Shaders::get("voxel-cascade-debug.frag"))
      ->output("outColor")
      ->link();
    // Setup Debug Rendering
    this->mesh = new Mesh();

    this->mesh
      ->edge(0, 1)
      ->edge(1, 2)
      ->edge(2, 3)
      ->edge(3, 0)
      
      ->edge(4, 5)
      ->edge(5, 6)
      ->edge(6, 7)
      ->edge(7, 4)

      ->edge(0, 4)
      ->edge(1, 5)
      ->edge(2, 6)
      ->edge(3, 7);

    this->mesh
      ->vert(0, 0, 0)
      ->vert(1, 0, 0)
      ->vert(1, 1, 0)
      ->vert(0, 1, 0)
      ->vert(0, 0, 1)
      ->vert(1, 0, 1)
      ->vert(1, 1, 1)
      ->vert(0, 1, 1);

    mesh->upload();

    size_t total_position_mem = this->level_count * 3 * sizeof(float) * BRICK_VOXEL_COUNT;
    float *positions = (float *)malloc(total_position_mem);

    size_t total_level_mem = this->level_count * sizeof(int32_t) * BRICK_VOXEL_COUNT;
    int32_t *levels_mem = (int32_t *)malloc(total_level_mem);
    size_t loc = 0;

    for (int levelIdx = 0; levelIdx <this->level_count; levelIdx++) {
      int cellSize = 1 << (levelIdx + 1);
      int radius = cellSize * BRICK_RADIUS;
      glm::vec3 v3CellSize = glm::vec3(cellSize);

      glm::vec3 level_lower = this->center - glm::vec3(radius);
      glm::vec3 level_upper = this->center + glm::vec3(radius);

      for (float x=-BRICK_RADIUS; x < BRICK_RADIUS; x++) {
        for (float y=-BRICK_RADIUS; y < BRICK_RADIUS; y++) {
          for (float z=-BRICK_RADIUS; z < BRICK_RADIUS; z++) {
            positions[loc * 3 + 0] = x;
            positions[loc * 3 + 1] = y;
            positions[loc * 3 + 2] = z;

            levels_mem[loc] = levelIdx;

            loc++;
          }
        }
      }
      printf("LEVELIDX %i - %f\n", levelIdx, float(levelIdx) / float(this->level_count));
    }
    // Instance data
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO); gl_error();
    glEnableVertexAttribArray(1); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_position_mem, &positions[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0); gl_error();
    glVertexAttribDivisor(1, 1); gl_error();
    
    // Level data
    unsigned int levelVBO;
    glGenBuffers(1, &levelVBO); gl_error();
    glEnableVertexAttribArray(2); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, levelVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_level_mem, &levels_mem[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(int32_t), 0); gl_error();
    glVertexAttribDivisor(2, 1); gl_error();
  }

  void debugRender(glm::mat4 mvp) {
    this->program->use()
      ->uniformVec3("center", this->center)
      ->uniform1i("total_levels", this->level_count)
      ->uniformMat4("mvp", mvp);

    glBindVertexArray(this->mesh->vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);


    glDrawElementsInstanced(
      GL_LINES,
      this->mesh->faces.size(),
      GL_UNSIGNED_INT,
      0,
      BRICK_VOXEL_COUNT * this->level_count
    );
  }
};