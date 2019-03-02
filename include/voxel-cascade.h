#pragma once

#include "volume.h"
#include <glm/glm.hpp>
#include <collision/aabb-obb.h>
#include "aabb.h"
#include "roaring.h"

struct SlabEntry {
  glm::mat4 invTransform; // 16 * 4
  glm::ivec4 brickIndex; // 16
  uint64_t brickData; // 8
  uint32_t volume_index; // 4
  //uint8_t _padding[4]; // 4
};

struct GPUCell {
  uint32_t state; // 0 empty, 0x01 has something
  uint32_t start;
  uint32_t end;
  uint32_t padding;
};


struct CPUCellBrick {
  Brick *brick;
  size_t volume_index;
};

struct CPUCell {
  vector<CPUCellBrick *> bricks;
  bool occupied;
};

struct CPULevel {
  CPUCell **cells;
  roaring_bitmap_t *occupancy_mask;
  size_t total_cells;
};
/*
class VoxelCascade {
  int level_count;

  CPULevel *levels;
  size_t total_cell_bytes = 0;
 
  size_t slab_size = 0;
  size_t slab_pos = 0;
  size_t total_slab_bytes = 0;

  GPUSlab *gpu_cascade_index;
  SSBO *ssbo_slab;

  glm::vec3 center;
  Mesh *debugLineMesh;
  Program *debugLineProgram;
  Program *debugRaytraceProgram;
  FullscreenSurface *debugRaytraceSurface;

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
    this->gpu_cascade_index = new GPUSlab(BRICK_VOXEL_COUNT * level_count * sizeof(GPUCell));

    // Setup the GPU cascade slab
    this->ssbo_slab = new SSBO(0);

    this->setupDebugRender();
  };

  void begin(glm::vec3 cascadeCenter) {
    this->center = cascadeCenter;

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
  };

  void addVolume(Volume *volume) {
    glm::mat4 mat = volume->getModelMatrix();
    
    glm::vec3 txVerts[8];
    glm::vec3 aabbVerts[8];
    glm::ivec3 lower(INT_MAX);
    glm::ivec3 upper(INT_MIN);
    glm::ivec3 offset;

    glm::vec3 offsets[8] = {
      glm::vec3(0.0, 1.0, 0.0),
      glm::vec3(1.0, 1.0, 0.0),
      glm::vec3(1.0, 0.0, 0.0),
      glm::vec3(0.0, 0.0, 0.0),

      glm::vec3(0.0, 1.0, 1.0),
      glm::vec3(1.0, 1.0, 1.0),
      glm::vec3(1.0, 0.0, 1.0),
      glm::vec3(0.0, 0.0, 1.0)
    };

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
        offset = offsets[i];
        
        txVerts[i] = txPoint(mat, brick->index + offset);
        lower.x = min(txVerts[i].x, float(lower.x));
        lower.y = min(txVerts[i].y, float(lower.y));
        lower.z = min(txVerts[i].z, float(lower.z));
 
        upper.x = max(txVerts[i].x, float(upper.x));
        upper.y = max(txVerts[i].y, float(upper.y));
        upper.z = max(txVerts[i].z, float(upper.z));
      }

      for (int level = 0; level < this->level_count; level++) {
        int cellSize = 1 << (level + 1);
        int prevCellSize = 1 << (level);
        int radius = cellSize * BRICK_RADIUS;
        glm::vec3 pos;
        glm::vec3 v3CellSize = glm::vec3(cellSize);

        glm::vec3 level_lower = this->center - glm::vec3(radius);
        glm::vec3 level_upper = this->center + glm::vec3(radius);

        // TODO: ensure the start/end lies within the current level
        if (!aabb_overlaps_component(level_lower, level_upper, lower, upper)) {
          continue;
        }

        glm::ivec3 start = glm::ivec3(
          glm::floor(glm::vec3(lower) / float(cellSize)) * float(cellSize)
        );
        glm::ivec3 end = glm::ivec3(
          glm::ceil(glm::vec3(upper) / float(cellSize)) * float(cellSize)
        );

        start.x = max(start.x, level_lower.x);
        start.y = max(start.y, level_lower.y);
        start.z = max(start.z, level_lower.z);

        end.x = min(end.x, level_upper.x);
        end.y = min(end.y, level_upper.y);
        end.z = min(end.z, level_upper.z);

        for (int x = start.x; x <= end.x; x+= cellSize) {
          pos.x = x;
          for (int y = start.y; y <= end.y; y+= cellSize) {
            pos.y = y;
            for (int z = start.z; z <= end.z; z+= cellSize) {
              pos.z = z;

              for (int vertIdx = 0; vertIdx < 8; vertIdx++) {
                aabbVerts[vertIdx] = pos + offsets[vertIdx] * v3CellSize;
              }

              // TODO: test if current cell intersects the obb
              bool isect = collision_aabb_obb(
                aabbVerts,
                txVerts
              );

              if (isect) {
                this->addBrickToCell(
                  brick,
                  level,
                  glm::ivec3(glm::round((pos - this->center + glm::vec3(radius)) / v3CellSize))
                  //glm::ivec3(BRICK_RADIUS) + (glm::ivec3(pos) / glm::ivec3(v3CellSize)) - glm::ivec3(this->center)
                );
              }
            }
          }
        }
      }
    }
  };

  void addBrickToCell(Brick *brick, int levelIdx, glm::ivec3 pos) {
    if (
      pos.x < 0 ||
      pos.y < 0 ||
      pos.z < 0 ||
      pos.x >= BRICK_DIAMETER ||
      pos.y >= BRICK_DIAMETER ||
      pos.z >= BRICK_DIAMETER
    ) {
      return;
    }

    int idx = (pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);

    CPUCell *cell = this->levels[levelIdx].cells[idx];
    CPUCellBrick b;
    b.brick = brick;
    cell->bricks.push_back(brick);
  };

  void end() {
    // Push each cell's bricks into contiguous memory while keeping the index to populate
    // the GPUCell's memory.
    this->ssbo_slab->resize(this->slab_size * sizeof(SlabEntry));
    SlabEntry *gpu_slab = (SlabEntry *)this->ssbo_slab->beginMap(SSBO::MAP_WRITE_ONLY);
    // TODO: we need to clear this slab or we're going to get ghosting when cells were filled
    //       but are no longer filled.
    GPUCell *gpu_cells = (GPUCell *)this->gpu_cascade_index->beginMap(GPUSlab::MAP_WRITE_ONLY);

    // TODO: there is a pretty large memory efficiency issue here and we should probably
    //       fix it by populating the slab via the highest level first and then walk up the
    //       levels and populate the start/end. This likely requires each slab region to be sorted
    //       and/or to store what level it intersects via a bitmask.
    for (int i = 0; i < level_count; i++) {
      size_t level_offset = BRICK_VOXEL_COUNT * i;    
      for (int j = 0; j < BRICK_VOXEL_COUNT; j++) {
        CPUCell *cpu_cell = this->levels[i].cells[j];
        size_t brick_count = cpu_cell->bricks.size();
        
        gpu_cells[level_offset + j].state = brick_count == 0 ? 0 : 1;
        gpu_cells[level_offset + j].start = this->slab_pos;
        gpu_cells[level_offset + j].end = this->slab_pos + brick_count;

        // Mark this cell as active
        if (brick_count != 0) {
          for (auto& brick : cpu_cell->bricks) {
            if (this->slab_pos < this->slab_size) {
              SlabEntry entry;
              //entry.brickData = brick->bufferAddress;
              entry.brickIndex = glm::ivec4(brick->index, 0);
              //entry.volume = 0;
              if (gpu_slab != nullptr) {
                memcpy(&gpu_slab[this->slab_pos], &entry, sizeof(entry));
              }
            }
            this->slab_pos++;
          }
        }
      }
    }
    this->ssbo_slab->endMap();
    this->gpu_cascade_index->endMap();
  };

  void setupDebugRender() {
    this->debugRaytraceSurface = new FullscreenSurface();
    this->debugRaytraceProgram = new Program();
    this->debugRaytraceProgram
      ->add(Shaders::get("voxel-cascade-debug-raytrace.vert"))
      ->add(Shaders::get("voxel-cascade-debug-raytrace.frag"))
      ->link();

    this->debugLineProgram = new Program();
    this->debugLineProgram
      ->add(Shaders::get("voxel-cascade-debug.vert"))
      ->add(Shaders::get("voxel-cascade-debug.frag"))
      ->output("outColor")
      ->link();
    // Setup Debug Rendering
    this->debugLineMesh = new Mesh();

    this->debugLineMesh
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

    this->debugLineMesh
      ->vert(0, 0, 0)
      ->vert(1, 0, 0)
      ->vert(1, 1, 0)
      ->vert(0, 1, 0)
      ->vert(0, 0, 1)
      ->vert(1, 0, 1)
      ->vert(1, 1, 1)
      ->vert(0, 1, 1);

    debugLineMesh->upload();

    size_t total_translation_mem = this->level_count * 3 * sizeof(float) * BRICK_VOXEL_COUNT;
    float *translations = (float *)malloc(total_translation_mem);

    size_t total_level_mem = this->level_count * sizeof(int32_t) * BRICK_VOXEL_COUNT;
    int32_t *levels_mem = (int32_t *)malloc(total_level_mem);
    size_t loc = 0;

    for (int levelIdx = 0; levelIdx <this->level_count; levelIdx++) {
      int cellSize = 1 << (levelIdx + 1);
      int radius = cellSize * BRICK_RADIUS;
      glm::vec3 v3CellSize = glm::vec3(cellSize);

      glm::vec3 level_lower = this->center - glm::vec3(radius);
      glm::vec3 level_upper = this->center + glm::vec3(radius);

      for (float x=0; x < BRICK_DIAMETER; x++) {
        for (float y=0; y < BRICK_DIAMETER; y++) {
          for (float z=0; z < BRICK_DIAMETER; z++) {
            translations[loc * 3 + 0] = x - BRICK_RADIUS;
            translations[loc * 3 + 1] = y - BRICK_RADIUS;
            translations[loc * 3 + 2] = z - BRICK_RADIUS;

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
    glBufferData(GL_ARRAY_BUFFER, total_translation_mem, &translations[0], GL_STATIC_DRAW); gl_error();
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
    this->debugLineProgram->use()
      ->uniformVec3("center", this->center)
      ->uniformMat4("mvp", mvp)
      ->bufferAddress("cascade_index", this->gpu_cascade_index->getAddress())
      ->ssbo("cascade_slab", this->ssbo_slab, 1);

    glBindVertexArray(this->debugLineMesh->vao); gl_error();
    glEnableVertexAttribArray(0); gl_error();
    glEnableVertexAttribArray(1); gl_error();
    glEnableVertexAttribArray(2); gl_error();


    glDrawElementsInstanced(
      GL_LINES,
      this->debugLineMesh->faces.size(),
      GL_UNSIGNED_INT,
      0,
      BRICK_VOXEL_COUNT * this->level_count
    );
    gl_error();
  }

  void debugRaytrace(glm::mat4 vp) {
    this->debugRaytraceProgram->use()
      ->uniformVec3("center", this->center)
      ->uniformMat4("VP", vp)
      ->bufferAddress("cascade_index", this->gpu_cascade_index->getAddress())
      ->ssbo("cascade_slab", this->ssbo_slab, 1);

    this->debugRaytraceSurface->render(this->debugRaytraceProgram);
  }
};
*/