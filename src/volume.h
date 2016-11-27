#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "gl-wrap.h"
#include "cl/clu.h"

#define DIMS 128

class Volume {
public:
  glm::vec3 center;
  GLbyte *data;
  GLuint textureId;
  cl_mem computeBuffer;

  Volume(glm::vec3 center) {
    this->center = center;
  }

  ~Volume() {
  }

  void upload (clu_job_t job) {
    glGenTextures(1, &this->textureId);
    gl_error();
    glBindTexture(GL_TEXTURE_3D, this->textureId);
    gl_error();

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage3D(
      GL_TEXTURE_3D,
      0,
      GL_RGB,
      DIMS,
      DIMS,
      DIMS,
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      0 // TODO: let opencl populate the data: this->data
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
  }

  void bind(Program *program) {
    glBindTexture(GL_TEXTURE_3D, this->textureId);
    glActiveTexture(GL_TEXTURE0);
    // TODO: setup center/aabb
    program->uniform1i("volume", 0)
           ->uniformVec3("center", this->center);
  }
};

#endif
