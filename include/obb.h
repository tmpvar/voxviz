#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#define TAU M_PI*2.0

#include <stdio.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

static glm::vec3 txOrthoNormal(glm::mat4 mat, glm::vec3 point) {
  glm::vec4 tmp = mat * glm::vec4(point, 0.0);
  return normalize(glm::vec3(tmp.x, tmp.y, tmp.z));
}

const glm::vec3 xnorm(1.0, 0.0, 0.0);
const glm::vec3 ynorm(0.0, 1.0, 0.0);
const glm::vec3 znorm(0.0, 0.0, 1.0);

class OBB {
  protected:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 radius;

    glm::mat3 orientation;

    bool dirty = true;
    glm::mat4 matrix;
  public:

    OBB() {
      this->position = glm::vec3(0.0);
      this->rotation = glm::vec3(0.0);
      this->radius = glm::vec3(0.0);
      this->clean();
    }

    OBB(glm::vec3 center, glm::vec3 radius, glm::vec3 rotation) {
      this->position = center;
      this->radius = radius;
      this->rotation = rotation;
      this->clean();
    }

    OBB(glm::vec3 center, glm::vec3 radius) {
      this->position = center;
      this->radius = radius;
      this->rotation = glm::vec3(0.0);
      this->clean();
    }

    OBB *move(glm::vec3 offset) {
      this->position += offset;
      this->dirty = true;
      return this;
    }

    OBB *rotate(glm::vec3 rotation) {
      this->rotation += rotation;
      this->dirty = true;
      return this;
    }

    OBB *setRotation(glm::vec3 rotation) {
      this->rotation = rotation;
      this->dirty = true;
      return this;
    }

    OBB *resize(glm::vec3 radius) {
      this->radius = radius;
      this->dirty = true;
      return this;
    }

    glm::mat3 getOrientation() {
      this->clean();
      return this->orientation;
    }

    glm::mat4 getMatrix() {
      this->clean();
      return this->matrix;
    }

    glm::vec3 getRadius() {
      return this->radius;
    }

    glm::vec3 getRotation() {
      return this->rotation;
    }

    glm::vec3 getPosition() {
      return this->position;
    }

  protected:
    void clean() {
      if (!this->dirty) {
        return;
      }

      // Update the 4x4 matrix
      {
        this->matrix = glm::mat4(1.0);
        this->matrix = glm::translate(this->matrix, this->position);

        glm::vec3 rot = this->rotation * glm::vec3(TAU);
        this->matrix = glm::rotate(this->matrix, this->rotation.x, glm::vec3(1.0, 0.0, 0.0));
        this->matrix = glm::rotate(this->matrix, this->rotation.y, glm::vec3(0.0, 1.0, 0.0));
        this->matrix = glm::rotate(this->matrix, this->rotation.z, glm::vec3(0.0, 0.0, 1.0));
      }

      // Rebuild the orthonormal obb orientation
      {
        orientation[0] = txOrthoNormal(this->matrix, xnorm);
        orientation[1] = txOrthoNormal(this->matrix, ynorm);
        orientation[2] = txOrthoNormal(this->matrix, znorm);
      }

      this->dirty = false;
    }
};