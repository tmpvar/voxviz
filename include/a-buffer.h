#pragma once

#include "core.h"
#include "gl-wrap.h"
#include "shaders/built.h"

class ABuffer {
public:
  Program *program = nullptr;
  Program *debugProgram = nullptr;
  int width = 0;
  int height = 0;
  int depth_complexity = 0;
  GLuint aBufferIndexTexture;
  GLuint aBufferTexture;

  ABuffer(int width, int height, int depth_complexity) {
    this->resize(width, height);
    this->depth_complexity = depth_complexity;

    this->program = new Program();
    this->program
      ->add(Shaders::get("a-buffer.vert"))
      ->add(Shaders::get("a-buffer.frag"))
      ->output("outColor")
      ->link();

    this->debugProgram = new Program();
    this->debugProgram
      ->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("a-buffer-debug.frag"))
      ->output("outColor")
      ->link();
  }

  ~ABuffer() {
    glDeleteBuffers(1, &this->aBufferIndexTexture);
    glDeleteBuffers(1, &this->aBufferTexture);
  }

  void resize(int width, int height) {
    if (width != this->width || height != this->height) {
      this->width = width;
      this->height = height;

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
        GL_R32F,
        width,
        height
      );
      gl_error();
      glBindTexture(GL_TEXTURE_2D, 0);
      gl_error();
    }
  }

  Program *begin() {
    this->program
      ->use();
      //->texture2dArray("aBuffer", this->aBufferTexture);
    
    //this->program->texture2d("aBufferIndex", this->aBufferIndexTexture);

     return this->program;
  }

  void end() {
    //Ensure that all global memory write are done before resolving
    //glMemoryBarrierEXT(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  }

  Program *debug() {
    return this->debugProgram
      ->use()
      ->texture2dArray("aBuffer", this->aBufferTexture);
  }
};