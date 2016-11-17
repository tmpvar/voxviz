#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <glm/glm.hpp>
#include "gl-wrap.h"

#define DIMS 128

class Volume {
public:
  glm::vec3 center;
  GLbyte *data;
  GLuint textureId;
  Volume(glm::vec3 center) {
    this->center = center;
    this->data = (GLbyte *)malloc(DIMS*DIMS*DIMS*3*sizeof(GLbyte));
  }

  ~Volume() {
    free(this->data);
  }

  void upload () {
    glGenTextures(1, &this->textureId);
    gl_error();
    glBindTexture(GL_TEXTURE_3D, this->textureId);
    gl_error();

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
      this->data
    );

    gl_error();
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
