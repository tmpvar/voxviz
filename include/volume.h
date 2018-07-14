#ifndef _VOLUME_H_
#define _VOLUME_H_

  #include <glm/glm.hpp>
  #include "clu.h"
  #include "aabb.h"

class Program;

  class Volume {
  public:
    glm::vec3 center;
    glm::uvec3 dims;
    unsigned int textureId;
    cl_mem computeBuffer;
    cl_mem mem_center;
    cl_mem mem_dims;
    float debug;

    Volume(glm::vec3 center, glm::uvec3 dims);
    ~Volume();
    void upload(clu_job_t job);
    void bind(Program * program);
    void position(float x, float y, float z);
    void move(float x, float y, float z);
    aabb_t aabb();
    bool isect(Volume *other, aabb_t *out);
  };
#endif
