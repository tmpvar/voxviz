#pragma once
#include "core.h"
#include "brick.h"
#include "gl-wrap.h"
#include <q3.h>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/integer.hpp"
#include "glm/gtx/hash.hpp"
#include <unordered_map>

class Volume {
protected:
  bool dirty;
public:
  unordered_map <glm::ivec3, Brick *>bricks;
  glm::vec3 rotation;
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec4 material;

  aabb_t *aabb;

  Mesh *mesh;

  q3Body* physicsBody;
  Volume(glm::vec3 pos, q3Scene *scene = nullptr, q3BodyDef *bodyDef = nullptr) {
    this->position = pos;
    this->rotation = glm::vec3(0.0);
    this->scale = glm::vec3(1.0);
    this->aabb = aabb_create();
    this->aabb->lower = glm::vec3(FLT_MAX);
    this->aabb->upper = glm::vec3(-FLT_MAX);

    this->dirty = false;

    this->mesh = new Mesh();

    this->mesh
      ->face(0, 1, 2)->face(0, 2, 3)
      ->face(4, 6, 5)->face(4, 7, 6)
      ->face(8, 10, 9)->face(8, 11, 10)
      ->face(12, 13, 14)->face(12, 14, 15)
      ->face(16, 18, 17)->face(19, 18, 16)
      ->face(20, 21, 22)->face(20, 22, 23);

    // TODO: move this into a "brick manager" or something
    // TODO: we can reuse the mesh data (vbo) but we probably need a
    //       volume specific vao.
    this->mesh
      ->vert(0, 0, 1)
      ->vert(1, 0, 1)
      ->vert(1, 1, 1)
      ->vert(0, 1, 1)
      ->vert(1, 1, 1)
      ->vert(1, 1, 0)
      ->vert(1, 0, 0)
      ->vert(1, 0, 1)
      ->vert(0, 0, 0)
      ->vert(1, 0, 0)
      ->vert(1, 1, 0)
      ->vert(0, 1, 0)
      ->vert(0, 0, 0)
      ->vert(0, 0, 1)
      ->vert(0, 1, 1)
      ->vert(0, 1, 0)
      ->vert(1, 1, 1)
      ->vert(0, 1, 1)
      ->vert(0, 1, 0)
      ->vert(1, 1, 0)
      ->vert(0, 0, 0)
      ->vert(1, 0, 0)
      ->vert(1, 0, 1)
      ->vert(0, 0, 1);

    #ifndef TESTING
      mesh->upload();
    #endif

    if (bodyDef != nullptr && scene != nullptr) {
      bodyDef->position.Set(pos.x, pos.y, pos.z);
      this->physicsBody = scene->CreateBody(*bodyDef);
    }
  }

  ~Volume() {
    for (auto& it : this->bricks) {
      Brick *brick = it.second;
      delete brick;
    }
    this->bricks.clear();
    delete this->mesh;
    aabb_free(this->aabb);
  }

  // TODO: consider denoting this as relative
  Brick *AddBrick(glm::ivec3 index, q3BoxDef *boxDef = nullptr) {
    Brick *brick = new Brick(index);
    this->bricks.emplace(index, brick);

    this->dirty = true;
    /*
    q3Transform tx;
    q3Identity(tx);
    tx.position.Set(
      float(index.x),
      float(index.y),
      float(index.z)
    );
    q3Vec3 extents(BRICK_DIAMETER, BRICK_DIAMETER, BRICK_DIAMETER);
    boxDef.Set(tx, extents);

    // TODO: associate this box w/ the brick
    this->physicsBody->AddBox(boxDef);
    */
    brick->volume = this;
    aabb_grow(this->aabb, brick->aabb);

    return brick;
  }

  Brick *getBrick(glm::ivec3 index) {
    auto it = this->bricks.find(index);
    Brick *found = nullptr;

    if (it != this->bricks.end()) {
      found = it->second;
    }

    return found;
  }


  Brick *getBrickFromWorldPos(glm::vec3 pos) {
    glm::ivec3 local = glm::ivec3(glm::floor(pos - this->position));
    return this->getBrick(local);
  }

  void move(float x, float y, float z) {
    this->position.x += x;
    this->position.y += y;
    this->position.z += z;
  }

  void bind() {
    // Mesh data
    if (!this->dirty) {
      glBindVertexArray(this->mesh->vao);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);
      return;
    }
    this->dirty = false;
    //this->mesh->upload();

    glBindVertexArray(this->mesh->vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->mesh->vbo);

    size_t total_bricks = this->bricks.size();
    if (total_bricks == 0) {
      return;
    }

    size_t total_position_mem = total_bricks * 3 * sizeof(float);
    size_t total_brick_pointer_mem = total_bricks * sizeof(GLuint64);
    float *positions = (float *)malloc(total_position_mem);
    GLuint64 *pointers = (GLuint64 *)malloc(total_brick_pointer_mem);
    size_t loc = 0;
    for (auto& it : this->bricks) {
      Brick *brick = it.second;
      positions[loc * 3 + 0] = float(brick->index.x);
      positions[loc * 3 + 1] = float(brick->index.y);
      positions[loc * 3 + 2] = float(brick->index.z);

      pointers[loc] = brick->bufferAddress;
      loc++;
    }

    // Instance data
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO); gl_error();
    glEnableVertexAttribArray(1); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_position_mem, &positions[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); gl_error();
    glVertexAttribDivisor(1, 1); gl_error();

    // Buffer pointer data
    unsigned int pointerVBO;
    glGenBuffers(1, &pointerVBO); gl_error();
    glEnableVertexAttribArray(2); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, pointerVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_brick_pointer_mem, &pointers[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribLPointer(2, 1, GL_UNSIGNED_INT64_ARB, 0, 0); gl_error();
    glVertexAttribDivisor(2, 1); gl_error();

    free(positions);
    free(pointers);
  }

  glm::mat4 getModelMatrix() {
    //    q3Transform tx = this->physicsBody->GetTransform();

    glm::mat4 model = glm::mat4(1.0f);

    //    model = glm::translate(model, glm::vec3(
    //      tx.position.x,
    //      tx.position.y,
    //      tx.position.z
    //    ));
    model = glm::translate(model, this->position);

    model = glm::rotate(model, this->rotation.x, glm::vec3(1.0, 0.0, 0.0));
    model = glm::rotate(model, this->rotation.y, glm::vec3(0.0, 1.0, 0.0));
    model = glm::rotate(model, this->rotation.z, glm::vec3(0.0, 0.0, 1.0));
    model = glm::scale(model, scale);

    return model;
  }

  bool overlaps(Volume *v) {
    glm::vec3 mylower = this->aabb->lower + this->position;
    glm::vec3 myupper = this->aabb->upper + this->position;

    glm::vec3 otherlower = v->aabb->lower + v->position;
    glm::vec3 otherupper = v->aabb->upper + v->position;

    // TODO: move this into aabb

    return (
      glm::all(glm::lessThanEqual(mylower, otherupper)) &&
      glm::all(glm::greaterThanEqual(myupper, otherlower))
      );
  }

  bool intersect(Volume *v, aabb_t *out) {
    aabb_t *mine = aabb_create();
    mine->lower = this->aabb->lower + this->position;
    mine->upper = this->aabb->upper + this->position;

    aabb_t *other = aabb_create();
    other->lower = v->aabb->lower + v->position;
    other->upper = v->aabb->upper + v->position;

    return aabb_isect(mine, other, out);
  }

  void computeBrickOffset(
    Brick *myBrick,
    Brick *otherBrick,
    glm::vec3 *myOffset,
    glm::vec3 *otherOffset,
    glm::ivec3 *sliceDims
  ) {
    Volume *otherVolume = otherBrick->volume;
    aabb_t *mine = aabb_create();
    aabb_t *other = aabb_create();
    aabb_t *overlap = aabb_create();
  
    mine->lower = this->position + glm::vec3(myBrick->index);
    mine->upper = mine->lower + glm::vec3(1.0);

    other->lower = otherVolume->position + glm::vec3(otherBrick->index);
    other->upper = other->lower + glm::vec3(1.0);

    aabb_isect(mine, other, overlap);
    
    glm::vec3 slice = overlap->upper - overlap->lower;

    sliceDims->x = int32_t(fabs(slice.x) * BRICK_DIAMETER);
    sliceDims->y = int32_t(fabs(slice.y) * BRICK_DIAMETER);
    sliceDims->z = int32_t(fabs(slice.z) * BRICK_DIAMETER);

    myOffset->x = floorf((overlap->lower.x - mine->lower.x) * (BRICK_DIAMETER));
    myOffset->y = floorf((overlap->lower.y - mine->lower.y) * (BRICK_DIAMETER));
    myOffset->z = floorf((overlap->lower.z - mine->lower.z) * (BRICK_DIAMETER));

    otherOffset->x = ceilf((overlap->lower.x - other->lower.x) * BRICK_DIAMETER);
    otherOffset->y = ceilf((overlap->lower.y - other->lower.y) * BRICK_DIAMETER);
    otherOffset->z = ceilf((overlap->lower.z - other->lower.z) * BRICK_DIAMETER);
  }

  void opCut(Volume *tool, Program *program = nullptr) {
    // TODO: return the number of affected voxels
    // TODO: add the bricks into a queue that we can process over a few
    //       frames so we don't jank on the current frame when the 
    //       complexity explodes.
    // check aabb intersection
    // check brick intersection
    // for every intersecting brick
    //   brick->cut(cutter.bricks[intersectingBrick])
    // TODO: get rid of this allocation
    aabb_t tmpAABB;
    bool overlaps = this->intersect(tool, &tmpAABB);
    if (!overlaps) {
      return;
    }

    glm::vec3 lower = tmpAABB.lower;
    glm::vec3 upper = tmpAABB.upper;
    // TODO: support for volume groups
    glm::vec3 pos;
    for (int x = lower.x; x <= upper.x; x++) {
      pos.x = x;
      for (int y = lower.y; y <= upper.y; y++) {
        pos.y = y;
        for (int z = lower.z; z <= upper.z; z++) {
          pos.z = z;
          /*Brick *toolBrick = tool->getBrickFromWorldPos(index);
          Brick *volumeBrick = this->getBrickFromWorldPos(index);
          if (toolBrick == nullptr || volumeBrick == nullptr) {
            continue;
          }*/
          //volumeBrick->cut(toolBrick, program);
          Brick *toolBrick = tool->getBrick(pos - tool->position);
          Brick *volumeBrick = this->getBrick(pos - this->position);

          if (toolBrick == nullptr || volumeBrick == nullptr) {
            continue;
          }

          glm::vec3 cutterOffset = ((this->position + glm::vec3(volumeBrick->index)) - pos) * glm::vec3(BRICK_DIAMETER);
          glm::vec3 volumeOffset = ((tool->position + glm::vec3(toolBrick->index)) - pos) * glm::vec3(BRICK_DIAMETER);

          program->use()
            ->bufferAddress("volume", volumeBrick->bufferAddress)
            ->bufferAddress("cutter", toolBrick->bufferAddress)
            ->uniformVec3i("volumeOffset", glm::ivec3(volumeOffset))
            ->uniformVec3i("cutterOffset", glm::ivec3(cutterOffset));

          glDispatchCompute(
            1,
            BRICK_DIAMETER,
            BRICK_DIAMETER
          );
        }
      }
    }
  }

  void opAdd(Volume *tool, Program *program = nullptr) {
    // TODO: return the number of affected voxels
    // TODO: add the bricks into a queue that we can process over a few
    //       frames so we don't jank on the current frame when the 
    //       complexity explodes.
    // check aabb intersection
    // check brick intersection
    // for every intersecting brick
    //   brick->cut(cutter.bricks[intersectingBrick])
    // TODO: get rid of this allocation
    aabb_t tmpAABB;
    bool overlaps = this->intersect(tool, &tmpAABB);
    glm::vec3 lower = tmpAABB.lower;
    glm::vec3 upper = tmpAABB.upper;

    if (!overlaps) {
      lower = floor(tool->aabb->lower + tool->position);
      upper = ceil(tool->aabb->upper + tool->position);
    }


    // TODO: support for volume groups
    glm::vec3 pos;
    for (int x = lower.x; x <= upper.x; x++) {
      pos.x = x;
      for (int y = lower.y; y <= upper.y; y++) {
        pos.y = y;
        for (int z = lower.z; z <= upper.z; z++) {
          pos.z = z;

          Brick *toolBrick = tool->getBrick(pos - tool->position);
          if (toolBrick == nullptr) {
            continue;
          }

          Brick *volumeBrick = this->getBrick(pos - this->position);

          if (volumeBrick == nullptr) {
            volumeBrick = this->AddBrick(pos - this->position);
            volumeBrick->createGPUMemory();
          }

          glm::vec3 toolOffset = ((this->position + glm::vec3(volumeBrick->index)) - pos) * glm::vec3(BRICK_DIAMETER);
          glm::vec3 volumeOffset = ((tool->position + glm::vec3(toolBrick->index)) - pos) * glm::vec3(BRICK_DIAMETER);

          program->use()
            ->bufferAddress("volumeBuffer", volumeBrick->bufferAddress)
            ->bufferAddress("toolBuffer", toolBrick->bufferAddress)
            ->uniformVec3i("volumeOffset", glm::ivec3(volumeOffset))
            ->uniformVec3i("toolOffset", glm::ivec3(toolOffset));

          glDispatchCompute(
            1,
            BRICK_DIAMETER,
            BRICK_DIAMETER
          );
        }
      }
    }
  }

};