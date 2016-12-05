#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "gl-wrap.h"
#include "clu.h"
#include "core.h"

class Volume {
public:
  glm::vec3 center;
  GLuint textureId;
  cl_mem computeBuffer;
  cl_mem mem_center;

  Volume(glm::vec3 center) {
    this->center = center;
  }

  ~Volume() {
    CL_CHECK_ERROR(clReleaseMemObject(this->mem_center));
  }

  void upload (clu_job_t job) {
    glGenTextures(1, &this->textureId);
    gl_error();
    glBindTexture(GL_TEXTURE_3D, this->textureId);
    gl_error();

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage3D(
      GL_TEXTURE_3D,
      0,
      GL_RGB,
      VOLUME_DIMS,
      VOLUME_DIMS,
      VOLUME_DIMS,
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      0
    );

    gl_error();

    cl_int shared_texture_error;

    // Create clgl shared texture
    this->computeBuffer = clCreateFromGLTexture(
      job.context,
      CL_MEM_WRITE_ONLY,
      GL_TEXTURE_3D,
      0,
      this->textureId,
      &shared_texture_error
    );

    clu_error(shared_texture_error);

    cl_int err;
    this->mem_center = clCreateBuffer(
      job.context,
      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      sizeof(this->center),
      &this->center[0],
      &err
    );
  }

  void bind(Program *program) {
    glBindTexture(GL_TEXTURE_3D, this->textureId);
    glActiveTexture(GL_TEXTURE0);
    glm::vec3 dims(VOLUME_DIMS, VOLUME_DIMS, VOLUME_DIMS);
    // TODO: setup center/aabb
    program->uniform1i("volume", 0)
           ->uniformVec3("center", this->center)
           ->uniformVec3("dims", dims);
  }
};

#endif
