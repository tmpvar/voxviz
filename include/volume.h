#pragma once
#include "core.h"
#include "brick.h"
#include "gl-wrap.h"
#include <q3.h>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/integer.hpp"
#include "glm/gtx/hash.hpp"
#include <unordered_map>
#include <queue>
#include <imgui.h>

class VolumeOperation {
public:
  Volume *tool;
  Brick *toolBrick;
  Brick *volumeBrick;
  Program *program;

  VolumeOperation(Volume *tool, Brick *toolBrick, Brick *volumeBrick, Program *program) {
    this->tool = tool;
    this->toolBrick = toolBrick;
    this->volumeBrick = volumeBrick;
    this->program = program;
  }
};


static glm::vec3 txPoint(glm::mat4 mat, glm::vec3 point) {
  glm::vec4 tmp = mat * glm::vec4(point, 1.0);
  return glm::vec3(tmp.x, tmp.y, tmp.z) / tmp.w;
}

static glm::vec3 txNormal(glm::mat4 mat, glm::vec3 point) {
  glm::vec4 tmp = mat * glm::vec4(point, 0.0);
  return glm::vec3(tmp.x, tmp.y, tmp.z);
}

static glm::vec3 vmin(glm::vec3 a, glm::vec3 b) {
  return glm::vec3(
    min(a.x, b.x),
    min(a.y, b.y),
    min(a.z, b.z)
  );
}

static glm::vec3 vmax(glm::vec3 a, glm::vec3 b) {
  return glm::vec3(
    max(a.x, b.x),
    max(a.y, b.y),
    max(a.z, b.z)
  );
}


class Volume {
protected:
  bool dirty;
public:
  unordered_map <glm::ivec3, Brick *>bricks;
  glm::vec3 rotation;
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec4 material;
  std::queue<VolumeOperation *> operation_queue;
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

  Brick *getBrick(glm::ivec3 index, bool createIfNotFound = false) {
    auto it = this->bricks.find(index);
    Brick *found = nullptr;

    if (it != this->bricks.end()) {
      found = it->second;
    }

    if (found == nullptr && createIfNotFound) {
      found = this->AddBrick(index);
      found->createGPUMemory();
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

  void opCut(Volume *tool, Program *program = nullptr) {
    
    // TODO: return the number of affected voxels
    // TODO: add the bricks into a queue that we can process over a few
    //       frames so we don't jank on the current frame when the 
    //       complexity explodes.
    // TODO: get rid of this allocation
    aabb_t tmpAABB;
    bool overlaps = this->intersect(tool, &tmpAABB);
    if (!overlaps) {
      return;
    }

    glm::vec3 lower = tmpAABB.lower - tool->position;
    glm::vec3 upper = tmpAABB.upper - tool->position;
    // TODO: support for volume groups
    glm::vec3 pos;
    Brick *toolBrick;
    for (int x = lower.x; x <= upper.x; x++) {
      for (int y = lower.y; y <= upper.y; y++) {
        for (int z = lower.z; z <= upper.z; z++) {
          // Volume index space
          toolBrick = tool->getBrick(glm::ivec3(x, y, z));
          // Volume world space
          pos = glm::vec3(x, y, z) + tool->position;

          glm::vec3 d = glm::floor(pos - this->position);

          for (uint8_t i = 0; i < 8; i++) {
            glm::vec3 corner = glm::vec3(
              i & 1 ? 1.0 : 0.0,
              i & 2 ? 1.0 : 0.0,
              i & 4 ? 1.0 : 0.0
            );
            this->addOperation(tool, toolBrick, this->getBrick(d + corner), program);
          }
        }
      }
    }
  }

  void obb(glm::vec3 *lower, glm::vec3 *upper) {
    glm::mat4 model = this->getModelMatrix();
    glm::vec3 l = txPoint(model, this->aabb->lower);
    glm::vec3 u = txPoint(model, this->aabb->upper);

    lower->x = fmin(l.x, u.x);
    lower->y = fmin(l.y, u.y);
    lower->z = fmin(l.z, u.z);

    upper->x = fmax(l.x, u.x);
    upper->y = fmax(l.y, u.y);
    upper->z = fmax(l.z, u.z);
  }

  void opAdd(Volume *tool, Program *program = nullptr) {
    // Index space bounding box
    glm::vec3 lower = tool->aabb->lower;
    glm::vec3 upper = tool->aabb->upper;
    glm::mat4 toolModel = tool->getModelMatrix();
    glm::mat4 toolToVolume = glm::inverse(this->getModelMatrix()) * tool->getModelMatrix();
    // TODO: support for volume groups
    glm::vec3 pos;
    Brick *toolBrick;
    for (auto& it : tool->bricks) {
      toolBrick = it.second;

      // Volume index space
      glm::vec3 brickIndex = toolBrick->index;

      // Compute the worldspace bounds that need to be processed for
      // this brick.
      glm::vec3 txBrickIndexLower = txPoint(toolToVolume, brickIndex);
      glm::vec3 txBrickIndexUpper = txPoint(toolToVolume, brickIndex + glm::vec3(1.0));
      glm::vec3 start = glm::floor(vmin(txBrickIndexLower, txBrickIndexUpper));
      glm::vec3 end = glm::ceil(vmax(txBrickIndexLower, txBrickIndexUpper));
      glm::vec3 step = txNormal(glm::inverse(toolModel), glm::vec3(1.0));
      step = glm::vec3(1.0);
      // TODO: handle negative scales?
      for (float i = start.x; i < end.x; i+=step.x ) {
        for (float j = start.y; j < end.y; j += step.y) {
          for (float k = start.z; k < end.z; k += step.z) {

            for (uint8_t idx = 0; idx < 8; idx++) {
              glm::vec3 corner = glm::vec3(
                idx & 1 ? 1.0 : 0.0,
                idx & 2 ? 1.0 : 0.0,
                idx & 4 ? 1.0 : 0.0
              );
              glm::vec3 volumeBrickIndex;

              volumeBrickIndex = corner + glm::vec3(i, j, k);

              Brick *volumeBrick = this->getBrick(volumeBrickIndex, true);
              //volumeBrick->fillConst(1.0);
              // TODO: the brick positioning code is basically correct, now it's time to 
              //       make the operation operate on the correct regions of each brick.
              this->addOperation(tool, toolBrick, volumeBrick, program);
            }
          }
        }
      }
    }
  }

  bool tick() {
    size_t size = this->operation_queue.size();
    if (size > 0) {
      VolumeOperation *op = this->operation_queue.front();
      this->operation_queue.pop();
      this->performOperation(op);
      return size == 1;
    }
    return true;
  }

  void addOperation(Volume *tool, Brick* volumeBrick, Brick *toolBrick, Program *program) {
    if (toolBrick == nullptr || volumeBrick == nullptr) {
      return;
    }
    VolumeOperation *op = new VolumeOperation(tool, volumeBrick, toolBrick, program);
    this->operation_queue.push(op);
  }

  void performOperation(VolumeOperation *op) {
    if (op == nullptr) {
      return;
    }

    /*
    OPS
     - transform the volume brick into world space
     - transform the volume brick by the inverted tool model
    
           +
         /   \
        /     \
       +       +
        \     /
      +--o---o
      |   \ /|
      |    + |
      +------+
    
     - find the transform/warp that turns a point in tool voxel space to volume voxel space
    
    */



    //glm::mat4 toolTransform = op->tool->getModelMatrix();
    //glm::mat4 invToolTransform = glm::inverse(toolTransform);
    //glm::mat4 volumeTransform = this->getModelMatrix();
    //glm::mat4 invVolumeTransform = glm::inverse(volumeTransform);
    //glm::mat4 toolToVolume = toolTransform * volumeTransform;
    
    //glm::vec3 toolBrickWorldPos = txPoint(toolTransform, glm::vec3(op->toolBrick->index));
    //glm::vec3 volumeBrickWorldPos = txPoint(volumeTransform, glm::vec3(op->volumeBrick->index));

    //glm::vec3 worldDiff = toolBrickWorldPos - volumeBrickWorldPos;
    //glm::vec3 toolOffset = worldDiff * glm::vec3(BRICK_DIAMETER);

    //glm::vec3 toolDirection = txNormal(toolToVolume, glm::vec3(0.0));
    //glm::vec3 otherToolDirection = txNormal(invToolTransform, glm::vec3(1.0));

    glm::mat4 volumeBrickModel = glm::mat4(1.0f);
    volumeBrickModel = glm::translate(volumeBrickModel, this->position + glm::vec3(op->volumeBrick->index));
    volumeBrickModel = glm::rotate(volumeBrickModel, this->rotation.x, glm::vec3(1.0, 0.0, 0.0));
    volumeBrickModel = glm::rotate(volumeBrickModel, this->rotation.y, glm::vec3(0.0, 1.0, 0.0));
    volumeBrickModel = glm::rotate(volumeBrickModel, this->rotation.z, glm::vec3(0.0, 0.0, 1.0));
    volumeBrickModel = glm::scale(volumeBrickModel, this->scale);

    glm::mat4 toolBrickModel = glm::mat4(1.0f);
    toolBrickModel = glm::translate(toolBrickModel, op->tool->position + glm::vec3(op->toolBrick->index));
    toolBrickModel = glm::rotate(toolBrickModel, op->tool->rotation.x, glm::vec3(1.0, 0.0, 0.0));
    toolBrickModel = glm::rotate(toolBrickModel, op->tool->rotation.y, glm::vec3(0.0, 1.0, 0.0));
    toolBrickModel = glm::rotate(toolBrickModel, op->tool->rotation.z, glm::vec3(0.0, 0.0, 1.0));
    toolBrickModel = glm::scale(toolBrickModel, op->tool->scale);

    glm::mat4 toolToVolume = glm::inverse(volumeBrickModel) * toolBrickModel;

    op->program->use()
      ->bufferAddress("volumeBuffer", op->volumeBrick->bufferAddress)
      ->bufferAddress("toolBuffer", op->toolBrick->bufferAddress)
      //->uniformVec3i("toolOffset", glm::ivec3(toolOffset))
      //->uniformVec3("toolDirection", otherToolDirection)
      //->uniformVec3("toolBrickIndex", op->toolBrick->index)
      //->uniformVec3("toolVolumeIndex", op->volumeBrick->index)
      ->uniformMat4("toolToVolume", toolToVolume);

    glDispatchCompute(1, BRICK_DIAMETER, BRICK_DIAMETER);
  }
};