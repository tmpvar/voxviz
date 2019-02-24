#pragma once

#include "volume.h"
#include <glm/glm.hpp>
#include <collision/aabb-obb.h>
#include "aabb.h"
#include "glm/glm.hpp"

#include "voxel-cascade.h"

class UniformGrid {
  CPULevel *cpu_grid;
  float cell_size;
  size_t total_cells;

  GPUSlab *gpu_grid;
  SSBO *gpu_slab;

  size_t slab_pos;
  size_t slab_size;
  bool dirty;

  glm::uvec3 dims;
  glm::uvec3 hdims;
  glm::vec3 center;

  FullscreenSurface *debugRaytraceSurface;
  Program *debugRaytraceProgram;

  public:
  UniformGrid(glm::uvec3 dims, float cell_size) {
    this->slab_size = 0;
    this->slab_pos = 0;
    this->dims = dims;
    this->dirty = true;

    this->cell_size = cell_size;
    this->total_cells = dims.x * dims.y * dims.z;
    size_t total_grid_bytes =  this->total_cells * sizeof(GPUCell);
    this->cpu_grid = new CPULevel;
    this->cpu_grid->cells = (CPUCell **)malloc(total_grid_bytes);
    this->cpu_grid->total_cells = this->total_cells;
    this->cpu_grid->occupancy_mask = roaring_bitmap_create();

    for (int j = 0; j < total_cells; j++) {
      this->cpu_grid->cells[j] = nullptr;
      //this->cpu_grid->cells[j]->occupied = false;
    }

    // Setup a container to manage the grid on the gpu
    this->gpu_grid = new GPUSlab(this->total_cells * sizeof(GPUCell));

    // setup a container to manage the grid cell contents on the gpu
    this->gpu_slab = new SSBO(0);


    // setup debug rendering 
    this->debugRaytraceSurface = new FullscreenSurface();
    this->debugRaytraceProgram = new Program();
    this->debugRaytraceProgram
      ->add(Shaders::get("uniform-grid-debug-raytrace.vert"))
      ->add(Shaders::get("uniform-grid-debug-raytrace.frag"))
      ->output("outColor")
      ->link();
  };

  ~UniformGrid() {
    delete this->cpu_grid;
    delete this->gpu_grid;
    delete this->gpu_slab;
  };

  bool begin(glm::vec3 gridCenter) {
  this->dirty = true;
    // TODO: if the incoming grid center (aka camera position)
    // is in the same cell as the previous frame, then we don't need
    // to clear and regen the grid.
    // However, making this changes makes it more difficult to update
    // dynamic objects and we will need a separate mechanism to perform
    // this type of update.

    glm::vec3 cellSize = glm::vec3(this->cell_size);
    bool cameraDirty = glm::any(
      glm::notEqual(
        glm::floor(gridCenter / cellSize),
        glm::floor(this->center / cellSize)
      )
    );

    if (true || cameraDirty) {
      this->center = glm::floor(gridCenter / cellSize);
      this->dirty = true;
    }

    // Ensure we have enough space to handle the next iteration.
    if (this->slab_pos > this->slab_size) {
      this->slab_size = (size_t)(ceilf(this->slab_pos / 1024.0f) * 1024);
      size_t total_slab_bytes = sizeof(SlabEntry) * this->slab_size;

      this->gpu_slab->resize(total_slab_bytes);
      this->dirty = true;
    }
    this->slab_pos = 0;


    if (!this->dirty) {
      return false;
    }

    roaring_uint32_iterator_t *i = roaring_create_iterator(this->cpu_grid->occupancy_mask);
    while (i->has_value) {
      this->cpu_grid->cells[i->current_value]->bricks.clear();
      this->cpu_grid->cells[i->current_value]->occupied = false;
      roaring_advance_uint32_iterator(i);
    }
    roaring_free_uint32_iterator(i);
    roaring_bitmap_clear(this->cpu_grid->occupancy_mask);
    return true;
  };

  void end() {
    // Push each cell's bricks into contiguous memory while keeping the index to populate
    // the GPUCell's memory.
    this->gpu_slab->resize(this->slab_size * sizeof(SlabEntry));
    SlabEntry *gpu_slab = (SlabEntry *)this->gpu_slab->beginMap(SSBO::MAP_WRITE_ONLY);
    GPUCell *gpu_cells = (GPUCell *)this->gpu_grid->beginMap(GPUSlab::MAP_WRITE_ONLY);

    // TODO: maybe clear this on the gpu side to avoid a full buffer transfer?
    memset(gpu_cells, 0, sizeof(GPUCell) * this->total_cells);
    roaring_uint32_iterator_t *i = roaring_create_iterator(this->cpu_grid->occupancy_mask);
    size_t occupied_cells = 0;
    this->slab_pos = 0;
    while (i->has_value) {
      occupied_cells ++;
      uint32_t j = i->current_value;
      CPUCell *cpu_cell = this->cpu_grid->cells[j];
      GPUCell *gpu_cell = &gpu_cells[j];
      size_t brick_count = cpu_cell->bricks.size();

      gpu_cell->state = 1;
      gpu_cell->start = this->slab_pos;
      gpu_cell->end = this->slab_pos + brick_count;

      for (auto& brick : cpu_cell->bricks) {
        if (this->slab_pos < this->slab_size) {
          gpu_slab[this->slab_pos].brickData = brick->bufferAddress;
          gpu_slab[this->slab_pos].brickIndex = glm::ivec4(brick->index, 0);
          
          glm::mat4 transform = brick->volume->getModelMatrix();
          glm::mat4 invTransform = glm::inverse(transform);
          memcpy(
            (void *)&gpu_slab[this->slab_pos].invTransform,
            (void *)glm::value_ptr(invTransform),
            sizeof(invTransform)
          );
          
          // TODO: populate this with the index into a global volume slab
          //gpu_slab[this->slab_pos].volume = 0;
        }
        this->slab_pos++;
      }

      roaring_advance_uint32_iterator(i);
    }
    ImGui::Text("occupied cells: %u", occupied_cells);
    roaring_free_uint32_iterator(i);

    this->gpu_grid->endMap();
    this->gpu_slab->endMap();

    this->dirty = false;
  };

  void addVolume(Volume *volume) {
    float cellSize = this->cell_size;
    glm::mat4 mat = volume->getModelMatrix();

    glm::vec3 txVerts[8];
    glm::vec3 aabbVerts[8];
    glm::vec3 lower(FLT_MAX);
    glm::vec3 upper(FLT_MIN);
    glm::vec3 offset;

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

    glm::vec3 v3CellSize = glm::vec3(cellSize);
    glm::vec3 radius = glm::vec3(this->dims) / glm::vec3(2);
    glm::vec3 pos;
    Brick *brick;

    //glm::ivec3 offset = txPoint(mat, brick->index + offset);
    for (auto& it : volume->bricks) {
      brick = it.second;
      lower.x = FLT_MAX;
      lower.y = FLT_MAX;
      lower.z = FLT_MAX;

      upper.x = -FLT_MAX;
      upper.y = -FLT_MAX;
      upper.z = -FLT_MAX;
      
      for (uint8_t i = 0; i < 8; i++) {
        offset = offsets[i];

        txVerts[i] = txPoint(mat, glm::vec3(brick->index) + offset);
        lower.x = min(txVerts[i].x, lower.x);
        lower.y = min(txVerts[i].y, lower.y);
        lower.z = min(txVerts[i].z, lower.z);

        upper.x = max(txVerts[i].x, upper.x);
        upper.y = max(txVerts[i].y, upper.y);
        upper.z = max(txVerts[i].z, upper.z);
      }
      
      glm::vec3 level_lower = this->center - radius * v3CellSize;
      glm::vec3 level_upper = this->center + radius * v3CellSize;

      // TODO: ensure the start/end lies within the current level
      if (!aabb_overlaps_component(level_lower, level_upper, lower, upper)) {
        continue;
      }

      glm::ivec3 start = glm::ivec3(
        glm::floor(lower / v3CellSize)
      );

      glm::ivec3 end = glm::ivec3(
        glm::ceil(upper / v3CellSize)
      );

      for (int x = start.x; x < end.x; x += 1) {
        pos.x = x;
        for (int y = start.y; y < end.y; y += 1) {
          pos.y = y;
          for (int z = start.z; z < end.z; z += 1) {
            pos.z = z;

            //for (int vertIdx = 0; vertIdx < 8; vertIdx++) {
            //  aabbVerts[vertIdx] = (pos + offsets[vertIdx]) * v3CellSize;
            //}

            // TODO: test if current cell intersects the obb
            //bool isect = collision_aabb_obb(
            //  aabbVerts,
            //  txVerts
            //);

            //if (true || isect) {
              this->addBrickToCell(
                brick,
                glm::ivec3(glm::floor(pos - this->center + radius))
              );
            //}
          }
        }
      }
    }
  };

  void addBrickToCell(Brick *brick, glm::ivec3 pos) {
    if (
      pos.x < 0 ||
      pos.y < 0 ||
      pos.z < 0 ||
      pos.x >= this->dims.x ||
      pos.y >= this->dims.y ||
      pos.z >= this->dims.z
      ) {
      return;
    }

    int idx = (pos.x + pos.y * this->dims.x + pos.z * this->dims.x * this->dims.y);

    if (this->cpu_grid->cells[idx] == nullptr) {
      this->cpu_grid->cells[idx] = new CPUCell;
      this->cpu_grid->cells[idx]->occupied = false;
    }

    CPUCell *cell = this->cpu_grid->cells[idx];
    
    cell->bricks.push_back(brick);
    
    if (!cell->occupied) {
      roaring_bitmap_add(this->cpu_grid->occupancy_mask, idx);
    }
    
    cell->occupied = true;
  };

  void debugRaytrace(glm::mat4 vp, glm::vec3 eye) {
    this->debugRaytraceProgram->use()
      ->uniformVec3("center", this->center)
      ->uniformVec3ui("dims", this->dims)
      ->uniformFloat("cellSize", this->cell_size)
      ->uniformVec3("eye", eye)
      ->uniformMat4("VP", vp)
      ->bufferAddress("uniformGridIndex", this->gpu_grid->getAddress())
      ->ssbo("uniformGridSlab", this->gpu_slab, 1);

    this->debugRaytraceSurface->render(this->debugRaytraceProgram);
  }

};