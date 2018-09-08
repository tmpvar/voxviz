#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "aabb.h"
#include "gl-wrap.h"

  class Program;

  class Brick {
  public:
    glm::vec3 center;
    glm::uvec3 dims;
    glm::vec3 rotation;
    // TODO: make this a structure
    GLuint bufferId;
    GLuint64 bufferAddress;
    float debug;

    Brick(glm::vec3 center, glm::uvec3 dims);
    ~Brick();
    void upload();
    void fill(Program * program);
    void bind(Program * program);
    glm::mat4 getModelMatrix();
    void position(float x, float y, float z);
    void move(float x, float y, float z);
    void rotate(float x, float y, float z);
    aabb_t aabb();
    bool isect(Brick *other, aabb_t *out);

    void fillConst(float val);
  };
#endif
