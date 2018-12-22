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
#include <limits>

class VolumeOperation {
public:
  Brick *toolBrick;
  Brick *stockBrick;
  glm::mat4 stockToTool;
  glm::vec3 toolVerts[8];
  Program *program;

  VolumeOperation(Brick *toolBrick, Brick *stockBrick, glm::mat4 stockToTool, glm::vec3 toolVerts[8], Program *program) {
    this->toolBrick = toolBrick;
    this->stockBrick = stockBrick;
    this->program = program;
    this->stockToTool = stockToTool;
    memcpy(this->toolVerts, toolVerts, sizeof(glm::vec3) * 8);
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

static bool collisionAABBvOBB(const glm::vec3 aabb[8], const glm::vec3 obb[8]) {
  glm::vec3 axes[6] = {
    glm::vec3(1.0, 0.0, 0.0),
    glm::vec3(0.0, 1.0, 0.0),
    glm::vec3(0.0, 0.0, 1.0),
    glm::normalize(obb[1] - obb[0]),
    glm::normalize(obb[2] - obb[0]),
    glm::normalize(obb[4] - obb[0])
  };
  double inf = std::numeric_limits<double>::infinity();
  glm::vec2 resultIntervals[2];

  for (int axisIdx = 0; axisIdx < 6; axisIdx++) {
    resultIntervals[0].x = inf;
    resultIntervals[0].y = -inf;
    resultIntervals[1].x = inf;
    resultIntervals[1].y = -inf;
    glm::vec3 axis = axes[axisIdx];

    for (int cornerIdx = 0; cornerIdx < 8; cornerIdx++) {
      auto aabbProjection = glm::dot(axis, aabb[cornerIdx]);
      auto obbProjection = glm::dot(axis, obb[cornerIdx]);

      resultIntervals[0].x = min(resultIntervals[0].x, aabbProjection);
      resultIntervals[0].y = max(resultIntervals[0].y, aabbProjection);

      resultIntervals[1].x = min(resultIntervals[1].x, obbProjection);
      resultIntervals[1].y = max(resultIntervals[1].y, obbProjection);
    }

    if (
      resultIntervals[1].x > resultIntervals[0].y ||
      resultIntervals[1].y < resultIntervals[0].x
      ) {
      return false;
    }
  }
  return true;
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
  size_t activeBricks;

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
      found->fillConst(0);
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

  bool occluded(glm::ivec3 pos) {
    // TODO: this could probably be optimized by using a bitset or similar
    Brick *tmp;

    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        for (int z = -1; z <= 1; z++) {

          if (x == 0 && y == 0 && z == 0) {
            continue;
          }

          tmp = this->getBrick(pos + glm::ivec3(x, y, z));
          if (tmp == nullptr) {
            return false;
          }

          if (!tmp->full) {
            return false;
          }
        }
      }
    }
    return true;
  }

  size_t bind() {
    // Mesh data
    if (!this->dirty) {
      glBindVertexArray(this->mesh->vao);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);
      return this->activeBricks;
    }
    this->dirty = false;
    //this->mesh->upload();

    glBindVertexArray(this->mesh->vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, this->mesh->vbo);

    size_t total_bricks = this->bricks.size();
    if (total_bricks == 0) {
      this->activeBricks = 0;
      return activeBricks;
    }

    size_t total_position_mem = total_bricks * 3 * sizeof(float);
    size_t total_brick_pointer_mem = total_bricks * sizeof(GLuint64);
    float *positions = (float *)malloc(total_position_mem);
    GLuint64 *pointers = (GLuint64 *)malloc(total_brick_pointer_mem);
    size_t loc = 0;
    for (auto& it : this->bricks) {
      Brick *brick = it.second;
      /*if (this->occluded(brick->index)) {
        continue;
      }*/

      positions[loc * 3 + 0] = float(brick->index.x);
      positions[loc * 3 + 1] = float(brick->index.y);
      positions[loc * 3 + 2] = float(brick->index.z);

      pointers[loc] = brick->bufferAddress;
      loc++;
    }

    cout << "loc: " << loc << endl;
    // Instance data
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO); gl_error();
    glEnableVertexAttribArray(1); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_position_mem, &positions[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0); gl_error();
    glVertexAttribDivisor(1, 1); gl_error();

    // Buffer pointer data
    unsigned int pointerVBO;
    glGenBuffers(1, &pointerVBO); gl_error();
    glEnableVertexAttribArray(2); gl_error();
    glBindBuffer(GL_ARRAY_BUFFER, pointerVBO); gl_error();
    glBufferData(GL_ARRAY_BUFFER, total_brick_pointer_mem, &pointers[0], GL_STATIC_DRAW); gl_error();
    glVertexAttribLPointer(2, 1, GL_UNSIGNED_INT64_ARB, sizeof(GLuint64), 0); gl_error();
    glVertexAttribDivisor(2, 1); gl_error();

    //free(positions);
    //free(pointers);
    this->activeBricks = loc;
    return this->activeBricks;
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

  void booleanOp(Volume *tool, bool additive, Program *program = nullptr) {
    glm::mat4 toolToStock = glm::inverse(this->getModelMatrix()) * tool->getModelMatrix();
    glm::mat4 stockToTool = glm::inverse(tool->getModelMatrix()) * this->getModelMatrix();

    glm::vec3 lower = glm::vec3(std::numeric_limits<double>::infinity());
    glm::vec3 upper = glm::vec3(-std::numeric_limits<double>::infinity());

    glm::vec3 toolVerts[8];
    glm::vec3 stockVerts[8];
    glm::vec3 offsets[8] = {
      glm::vec3(0.0, 1.0, 0.0),
      glm::vec3(1.0, 1.0, 0.0),
      glm::vec3(1.0, 0.0, 0.0),
      glm::vec3(0.0, 0.0, 0.0),

      glm::vec3(0.0, 1.0, 1.0),
      glm::vec3(1.0, 1.0, 1.0),
      glm::vec3(1.0, 0.0, 1.0),
      glm::vec3(0.0, 0.0, 1.0)
    };

    Brick *toolBrick;
    for (auto& it : tool->bricks) {
      toolBrick = it.second;

      for (int vertIdx = 0; vertIdx < 8; vertIdx++) {
        toolVerts[vertIdx] = txPoint(toolToStock, glm::vec3(toolBrick->index) + offsets[vertIdx]);

        // maintain the extents of the transformed verts in volume space
        lower.x = min(lower.x, toolVerts[vertIdx].x);
        lower.y = min(lower.y, toolVerts[vertIdx].y);
        lower.z = min(lower.z, toolVerts[vertIdx].z);
        upper.x = max(upper.x, toolVerts[vertIdx].x);
        upper.y = max(upper.y, toolVerts[vertIdx].y);
        upper.z = max(upper.z, toolVerts[vertIdx].z);
      }


      glm::ivec3 start = glm::floor(lower);
      glm::ivec3 end = glm::ceil(upper);

      for (int x = start.x; x < end.x; x++) {
        for (int y = start.y; y < end.y; y++) {
          for (int z = start.z; z < end.z; z++) {
            glm::ivec3 brickIndex = glm::ivec3(x, y, z);

            for (int vertIdx = 0; vertIdx < 8; vertIdx++) {
              stockVerts[vertIdx] = glm::vec3(brickIndex) + offsets[vertIdx];
            }

            if (collisionAABBvOBB(stockVerts, toolVerts)) {
              Brick *stockBrick = this->getBrick(brickIndex, additive);
              if (!stockBrick) {
                continue;
              }
              this->addOperation(toolBrick, stockBrick, stockToTool, toolVerts, program);
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

  void addOperation(Brick* volumeBrick, Brick *toolBrick, glm::mat4 stockToTool, glm::vec3 toolVerts[8], Program *program) {
    if (toolBrick == nullptr || volumeBrick == nullptr) {
      return;
    }
    VolumeOperation *op = new VolumeOperation(volumeBrick, toolBrick, stockToTool, toolVerts, program);
    this->operation_queue.push(op);
  }

  void performOperation(VolumeOperation *op) {
    if (op == nullptr) {
      return;
    }

    op->program->use()
      ->bufferAddress("stockBuffer", op->stockBrick->bufferAddress)
      ->bufferAddress("toolBuffer", op->toolBrick->bufferAddress)
      ->uniformVec3("toolBrickIndex", glm::vec3(op->toolBrick->index))
      ->uniformVec3("stockBrickIndex", glm::vec3(op->stockBrick->index))
      ->uniformVec3fArray("toolBrickVerts", op->toolVerts, 8)
      ->uniformMat4("stockToTool", op->stockToTool);

    glDispatchCompute(BRICK_VOXEL_WORDS, 1, 1);
    gl_error();
  }
};