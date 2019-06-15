#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/integer.hpp"
#include "glm/gtx/hash.hpp"

#include "obb.h"
#include "parser/magicavoxel/vox.h"

class Entity {
  protected:
    OBB *obb;
    bool matrix_dirty = true;
    glm::mat4 matrix_cached;

  public:
    Entity() {
      this->obb = new OBB();
    }

    Entity(glm::vec3 position, glm::vec3 radius) {
      this->obb = new OBB(position, radius);
    }

    Entity(glm::vec3 position) {
      this->obb = new OBB(position, glm::vec3(1.0));
    }

    void tick(double deltaTime) { /* NOOP */ }

    void move(glm::vec3 offset) {
      this->obb->move(offset);
    }

    void rotate(glm::vec3 offset) {
      this->obb->rotate(offset);
    }

    void setRotation(glm::vec3 rotation) {
      this->obb->setRotation(rotation);
    }

    void clampPosition(glm::vec3 lower, glm::vec3 upper) {
      this->obb->clampPosition(lower, upper);
    }

    glm::vec3 getPosition() {
      return this->obb->getPosition();
    }
    

    void paintInto(uint32_t *buf, glm::uvec3 bufDims) {
      assert(false);
    }
};


class VoxEntity : public Entity {
  VOXModel *vox;

  public:
    VoxEntity(const char *filename, glm::vec3 position)
    : Entity::Entity(position) 
    {
      this->vox = VOXParser::parse(filename);

      this->obb->resize(glm::vec3(this->vox->dims) / glm::vec3(2.0));
    }

    ~VoxEntity() {
      if (this->vox != nullptr) {
        delete this->vox;
      }
    }

    void paintInto(uint8_t *buf, glm::uvec3 bufDims) {
      glm::vec3 radius = this->obb->getRadius();
      glm::vec3 center = this->obb->getPosition();
      glm::uvec3 dims(radius * glm::vec3(2.0));

      glm::ivec3 offset = -radius;//glm::ivec3(center - radius);

      size_t bufLength = bufDims.x * bufDims.y * bufDims.z;
      glm::mat4 mat = this->obb->getMatrix();

      uint64_t src_idx = 0;
      uint64_t dimsxy = dims.x * dims.y;
      uint64_t zoff = 0;
      uint64_t yoff = 0;

      for (int32_t z = 0; z < dims.z; z++) {
        zoff = z * dimsxy;
        for (int32_t y = 0; y < dims.y; y++) {
          yoff = y * dims.x;
          for (int32_t x = 0; x < dims.x; x++) {

            src_idx++;

            glm::vec4 tmp(
              static_cast<float>(x + offset.x) + 0.5,
              static_cast<float>(y + offset.y) + 0.5,
              static_cast<float>(z + offset.z) + 0.5,
              1.0
            );
            tmp = mat * tmp;
            glm::ivec3 pos = glm::ivec3(
              glm::floor(glm::vec3(tmp.x, tmp.y, tmp.z) / tmp.w + glm::vec3(0.5))
            );


            uint64_t src_idx = (x + yoff + zoff);

            int64_t dest_idx = (
              pos.x +
              pos.y * bufDims.x +
              pos.z * bufDims.x * bufDims.y
            );

            if (dest_idx < 0 || dest_idx >= bufLength) {
              continue;
            }

            buf[dest_idx] = vox->buffer[src_idx];
          }
        }
      }
    }
};


class Scene {
  public:
};

