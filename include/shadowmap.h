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
  glm::vec3 invEye;
  glm::vec3 target;
  Program *program;
  FreeCamera *camera;

  Shadowmap() {
    this->program = new Program();

    this->program
      ->add(Shaders::get("raytrace.vert"))
      ->add(Shaders::get("raytrace-light.frag"))
      ->output("outColor")
      ->output("outPosition")
      ->link();

    this->program->use();
  }

  ~Shadowmap() {
  }

  void begin() {
    this->depthViewMatrix = glm::lookAt(this->eye, this->target, glm::vec3(0.0, 1.0, 0.0));
    this->depthMVP = depthProjectionMatrix * depthViewMatrix;

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

  void end() {}

  void orient(glm::vec3 eye, glm::vec3 target) {
    this->eye = eye;
    this->target = target;
  }
};