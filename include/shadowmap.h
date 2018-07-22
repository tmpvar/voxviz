#pragma once

#include "gl-wrap.h"
#include "glm/glm.hpp"
#include "camera-free.h"

class Shadowmap {
public:
  //GLuint fbo = 0;
  GLuint depthTexture;
  glm::mat4 depthProjectionMatrix;
  glm::mat4 depthViewMatrix;
  glm::mat4 depthModelMatrix;
  glm::mat4 depthMVP;
  glm::vec3 eye;
  Program *program;
  FreeCamera *camera;

  Shadowmap() {
    this->program = new Program();

    this->program
      ->add(Shaders::get("raytrace-light.vert"))
      ->add(Shaders::get("raytrace-light.frag"))
      ->output("outColor");
    
    this->program->link();

    this->camera = new FreeCamera();
    camera->translate(1024, -2048, -512);
    camera->rotate(1.0f, 0.75f, 1.0f, 0.0f);
    camera->rotate(1.0f, 0.0f, 0.25f, 0.75f);

    /*
    glGenFramebuffers(1, &this->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    
    glGenTextures(1, &this->depthTexture);
    glBindTexture(GL_TEXTURE_2D, this->depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->depthTexture, 0);
    glDrawBuffer(GL_NONE); // No color buffer is drawn to.

                           // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      gl_error();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    */
  }

  ~Shadowmap() {
    //glDeleteTextures(1, &this->depthTexture);
    //glDeleteFramebuffers(1, &this->fbo);
  }

  void bindCollect(Program *p, FBO *fbo) {
    glm::vec3 target = glm::vec3(0,0,0);

    // Compute the MVP matrix from the light's point of view
    
    //camera->translate(-1024, -2048, -2048);
    //camera->rotate(1.0f, 0.75f, 0.0f, 0.0f);

    //this->depthProjectionMatrix = glm::ortho<float>(-1000, 1000, 1000, -1000, 0.1, 2000);
    //this->depthViewMatrix = glm::lookAt(
    //  target,
    //  glm::vec3(1024, 2048, 2048),
    //  glm::vec3(0, 1, 0)
    //);
    this->depthViewMatrix = this->camera->view();

    this->depthModelMatrix = glm::mat4(1.0);
    this->depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
    glm::mat4 invertedView = glm::inverse(depthViewMatrix);
    this->eye = glm::vec3(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
  }

  void bindUse(Program *p) {
    glm::mat4 biasMatrix(
      0.5, 0.0, 0.0, 0.0,
      0.0, 0.5, 0.0, 0.0,
      0.0, 0.0, 0.5, 0.0,
      0.5, 0.5, 0.5, 1.0
    );
    glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

    p->uniformMat4("depthBiasMVP", depthBiasMVP);
  } 
};