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
  glm::mat4 depthBiasMVP;
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
  }

  ~Shadowmap() {
  }

  void bindCollect(Program *p, FBO *fbo) {
    glm::vec3 target = glm::vec3(0,0,0);
    this->depthViewMatrix = this->camera->view();
    this->depthMVP = depthProjectionMatrix * depthViewMatrix * glm::mat4(1.0);

    glm::mat4 invertedView = glm::inverse(depthViewMatrix);
    this->eye = glm::vec3(invertedView[3][0], invertedView[3][1], invertedView[3][2]);

    glm::mat4 biasMatrix(
      0.5, 0.0, 0.0, 0.0,
      0.0, 0.5, 0.0, 0.0,
      0.0, 0.0, 0.5, 0.0,
      0.5, 0.5, 0.5, 1.0
    );
    
    this->depthBiasMVP = biasMatrix * this->depthMVP;
  }
};