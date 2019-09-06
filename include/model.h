#pragma once

#include "gl-wrap.h"
#include "greedy-mesher.h"
#include "parser/magicavoxel/vox.h"

#include <glm/glm.hpp>
#include <string>

using namespace std;
using namespace glm;

class Model {
  u32 normalVBO;

  Model(VOXModel *m) {
    this->vox = m;
    this->mesh = new Mesh();

    //greedy(this->vox->buffer, ivec3(this->vox->dims), this->mesh);
    culled_mesher(this->vox->buffer, ivec3(this->vox->dims), this->mesh);

    this->mesh->upload();

    glGenBuffers(1, &this->normalVBO); gl_error();
    glEnableVertexAttribArray(1); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO); gl_error();
    glBufferData(
      GL_ARRAY_BUFFER,
      this->mesh->normals.size() * sizeof(GLfloat),
      (GLfloat *)this->mesh->normals.data(),
      GL_STATIC_DRAW
    );
    gl_error();
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0); gl_error();
    glVertexAttribDivisor(1, 0); gl_error();

    this->matrix = mat4(1.0);
    u32 total_bytes = this->vox->dims.x * this->vox->dims.y * this->vox->dims.z;

    this->data = new SSBO(
      total_bytes,
      this->vox->dims
    );

    this->dims = this->vox->dims;

    uint8_t *buf = (uint8_t *)this->data->beginMap(SSBO::MAP_WRITE_ONLY);
    memcpy(buf, this->vox->buffer, total_bytes);
    this->data->endMap();


    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gColorSpec;


  }

public:

  enum SourceModelType {
    Unknown = 0,
    MagicaVoxel
  };

  u8 *buffer;
  uvec3 dims;
  Mesh *mesh;
  SSBO *data;
  VOXModel *vox;
  SourceModelType source_model_type = SourceModelType::Unknown;
  mat4 matrix;

  static Model *New(string filename) {
    VOXModel *vox = VOXParser::parse(filename);
    if (vox == nullptr) {
      cout << "cannot load " << filename << endl;
      return nullptr;
    }
    return new Model(vox);
  }

  void render(Program *program, mat4 VP) {
    program
      ->uniform1ui("totalTriangles", this->mesh->faces.size())
      ->uniformMat4("model", this->matrix);

    glBindVertexArray(this->mesh->vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawElements(
      GL_TRIANGLES,
      (GLsizei)this->mesh->faces.size(),
      GL_UNSIGNED_INT,
      0
    );

    //this->mesh->render(program);
  }
};

class GBuffer {
  uvec2 res;

  public:
    GLuint gPosition;
    GLuint gNormal;
    GLuint gColor;
    GLuint gDepth;

    GLuint gBuffer;

    GBuffer() {
      this->res = uvec2(0);
      this->gBuffer = 0xFFFFFFFF;
    }

    GBuffer *resize(uvec2 resolution) {
      if (resolution.x == this->res.x && resolution.y == this->res.y) {
        return this;
      }

      this->res = resolution;

      if (this->gBuffer != 0xFFFFFFFF) {
        glDeleteFramebuffers(1, &this->gBuffer);
        glDeleteTextures(1, &this->gPosition);
        glDeleteTextures(1, &this->gNormal);
        glDeleteTextures(1, &this->gColor);
        glDeleteTextures(1, &this->gBuffer);
      }

      glGenFramebuffers(1, &this->gBuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, this->gBuffer);

      gl_error();

      // - color buffer
      glGenTextures(1, &gColor);
      glBindTexture(GL_TEXTURE_2D, this->gColor);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->res.x, this->res.y, 0, GL_RGB, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gColor, 0);
      gl_error();

      // - position buffer
      glGenTextures(1, &this->gPosition);
      glBindTexture(GL_TEXTURE_2D, this->gPosition);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->res.x, this->res.y, 0, GL_RGB, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, this->gPosition, 0);
      gl_error();

      // - normal buffer
      glGenTextures(1, &this->gNormal);
      glBindTexture(GL_TEXTURE_2D, this->gNormal);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, this->res.x, this->res.y, 0, GL_RGB, GL_FLOAT, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, this->gNormal, 0);
      gl_error();

      // - depth buffer
      glGenTextures(1, &this->gDepth);
      glBindTexture(GL_TEXTURE_2D, this->gDepth);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->res.x, this->res.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->gDepth, 0/*mipmap level*/);
      gl_error();

      // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
      unsigned int attachments[3] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2
      };
      glDrawBuffers(3, attachments);
      gl_error();
      return this;
    }

    GBuffer *bind() {
      glBindFramebuffer(GL_FRAMEBUFFER, this->gBuffer);
      return this;
    }

    GBuffer *unbind() {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      return this;
    }

    void debugRender() {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, this->gBuffer);

      glBlitFramebuffer(
        0,
        0,
        this->res.x,
        this->res.y,
        0,
        0,
        this->res.x,
        this->res.y,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
      );
    }

};