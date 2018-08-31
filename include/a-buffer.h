#pragma once

#include "core.h"
#include "gl-wrap.h"
#include "shaders/built.h"
#include "fullscreen-surface.h"

#include "glm/glm.hpp"

class ABuffer {
public:
  Program *program = nullptr;
  Program *debugProgram = nullptr;
  Program *clearProgram = nullptr;
  glm::ivec2 texture_dims;
  int depth_complexity = 0;
  GLuint aBufferIndexTexture;
  GLuint aBufferTexture;
  FullscreenSurface *fsSurface;

  ABuffer(int width, int height, int depth_complexity) {
    this->resize(width, height);
    this->depth_complexity = depth_complexity;

    this->program = new Program();
    this->program
      ->add(Shaders::get("a-buffer.vert"))
      ->add(Shaders::get("a-buffer.frag"))
      ->output("outColor")
      ->link();

    this->clearProgram = new Program();
    this->clearProgram
      ->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("a-buffer-clear.frag"))
      ->link();

    this->debugProgram = new Program();
    this->debugProgram
      ->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("a-buffer-debug.frag"))
      ->output("outColor")
      ->link();

    this->fsSurface = new FullscreenSurface();
  }

  ~ABuffer() {
    glDeleteBuffers(1, &this->aBufferIndexTexture);
    glDeleteBuffers(1, &this->aBufferTexture);
  }

  void resize(int width, int height) {
    cout << "abuffer.resize" << endl;
    if (width != this->texture_dims.x || height != this->texture_dims.y) {
      cout << "rebuild abuffer" << endl;
      this->texture_dims.x = width;
      this->texture_dims.y = height;

      glDeleteBuffers(1, &this->aBufferIndexTexture);
      glDeleteBuffers(1, &this->aBufferTexture);

      // build textures
      // TODO: consider using bindless global buffers

      // position/id buffer      
      glGenTextures(1, &this->aBufferTexture);
      //glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, this->aBufferTexture);
      //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexStorage3D(
        GL_TEXTURE_2D_ARRAY,
        1,
        GL_RGBA32F,
        width,
        height,
        ABUFFER_MAX_DEPTH_COMPLEXITY
      );
//      gl_error();
//      glBindImageTexture(0, this->aBufferTexture, 0, true, 0,  GL_READ_WRITE, GL_RGBA32F);
      gl_error();
      glBindTexture(GL_TEXTURE_2D, 0);
      gl_error();

      // index buffer
      glGenTextures(1, &this->aBufferIndexTexture);

      glBindTexture(GL_TEXTURE_2D, this->aBufferIndexTexture);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexStorage2D(
        GL_TEXTURE_2D,
        1,
        GL_R32UI,
        width,
        height
      );
      gl_error();
      glBindTexture(GL_TEXTURE_2D, 0);
      gl_error();
    }
  }

  Program *begin() {
    glBindImageTexture(3, this->aBufferTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(4, this->aBufferIndexTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
   
    this->fsSurface->render(this->clearProgram->use());
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return this->program->use();
  }

  void end() {
    //Ensure that all global memory write are done before resolving
    glMemoryBarrierEXT(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }

  Program *debug(int slice) {
    glBindImageTexture(3, this->aBufferTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    return this->debugProgram->use()->uniform1i("slice", slice);
  }
};