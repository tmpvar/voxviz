#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "aabb.h"
#include "gl-wrap.h"

  class Program;

  class Brick {
  public:
    glm::ivec3 index;
    // TODO: make this a structure
    GLuint bufferId;
    GLuint64 bufferAddress;
    float debug;
    float *data;

    Brick(glm::ivec3 index);
    ~Brick();
    void createGPUMemory();
    void upload();
    void fill(Program * program);
    void bind(Program * program);
    aabb_t aabb();
    bool isect(Brick *other, aabb_t *out);

    void setVoxel(glm::uvec3 pos, float val);
    void fillConst(float val);
  };
#endif
