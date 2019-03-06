#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "aabb.h"
#include "gl-wrap.h"
  
#define BRICK_MEM_TYPE GL_SHADER_STORAGE_BUFFER

  class Program;
  class Volume;

  class Brick {
  public:
    glm::ivec3 index;
    // TODO: make this a structure
    GLuint bufferId;
    GLuint64 bufferAddress;
    float debug;
    float *data;
    aabb_t *aabb;
    Volume *volume;
    bool full;
    Brick(glm::ivec3 index);
    ~Brick();
    void createGPUMemory();
    void upload();
    void fill(Program * program);
    void bind(Program * program);
    bool isect(Brick *other, aabb_t *out);

    void setVoxel(glm::uvec3 pos, float val);
    void fillConst(uint32_t val);
    void cut(Brick *cutter, Program *program);
  };
#endif
