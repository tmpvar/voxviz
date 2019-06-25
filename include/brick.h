#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "aabb.h"
#include "gl-wrap.h"
  

  class Program;
  class Volume;

  struct BrickData {
    uint32_t data[BRICK_VOXEL_WORDS];
  };

  class BrickMemory {
  private:
    uint32_t alloc_head = 0;
    static BrickMemory *_instance;
  public:
    SSBO *ssbo;
    BrickMemory();
    uint32_t alloc();
    static BrickMemory *instance();
  };


  class Brick {
  public:
    glm::ivec3 index;
    uint32_t memory_index = 0xFFFFFFFF;
    float debug;
    uint32_t *data = nullptr;
    aabb_t *aabb;
    Volume *volume;
    bool full;
    Brick(glm::ivec3 index);
    ~Brick();
    void createGPUMemory();
    void fill(Program * program);
    void bind(Program * program);
    bool isect(Brick *other, aabb_t *out);

    void setVoxel(glm::uvec3 pos);
    void fillConst(uint32_t val);
    void upload();

    void createHostMemory() {
      if (this->data != nullptr) {
        return;
      }
      this->data = (uint32_t *)malloc(BRICK_VOXEL_BYTES);
      memset(this->data, 0, BRICK_VOXEL_BYTES);
    }

    void freeHostMemory() {
      if (this->data != nullptr) {
        free(this->data);
        this->data = nullptr;
      }

    }

  };
#endif
