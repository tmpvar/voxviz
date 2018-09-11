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
  Mesh *mesh;

  q3Body* physicsBody;
  Volume(glm::vec3 pos, q3Scene *scene, q3BodyDef bodyDef) {
    this->position = pos;
    this->rotation = glm::vec3(0.0);
    this->scale = glm::vec3(1.0);

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
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER)
      ->vert(1 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->vert(0 * BRICK_DIAMETER, 0 * BRICK_DIAMETER, 1 * BRICK_DIAMETER)
      ->upload();

    bodyDef.position.Set(pos.x, pos.y, pos.z);
    this->physicsBody = scene->CreateBody(bodyDef);
    
  }

  ~Volume() {
    for (auto& it : this->bricks) {
      Brick *brick = it.second;
      delete brick;
    }
    this->bricks.clear();
    delete this->mesh;
  }

  // TODO: consider denoting this as relative
  Brick *AddBrick(glm::ivec3 index, q3BoxDef boxDef) {
    Brick *brick = new Brick(index);
    this->bricks.emplace(index, brick);

    this->dirty = true;
    q3Transform tx;
    q3Identity(tx);
    tx.position.Set(
      float(index.x * BRICK_DIAMETER),
      float(index.y * BRICK_DIAMETER),
      float(index.z * BRICK_DIAMETER)
    );
    q3Vec3 extents(BRICK_DIAMETER, BRICK_DIAMETER, BRICK_DIAMETER);
    boxDef.Set(tx, extents);

    // TODO: associate this box w/ the brick
    this->physicsBody->AddBox(boxDef);

    return brick;
  }

  void setVoxel(glm::ivec3 coord, float val) {

    glm::ivec3 index = glm::ivec3(
      floor(coord.x / BRICK_DIAMETER),
      floor(coord.y / BRICK_DIAMETER),
      floor(coord.z / BRICK_DIAMETER)
    );

    // find the brick that contains this voxel
    // if no brick, create one
    Brick *found = nullptr;

    auto it = this->bricks.find(index);
    if (it != this->bricks.end()) {
      found = it->second;
    }
    else {
      q3BoxDef boxDef;
      q3Transform tx;
      q3Identity(tx);
      boxDef.SetRestitution(0.5);
      found = this->AddBrick(index, boxDef);
    }
    found->setVoxel(mod(glm::vec3(coord), glm::vec3(BRICK_DIAMETER)), val);
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
      positions[loc * 3 + 0] = float(brick->index.x * BRICK_DIAMETER);
      positions[loc * 3 + 1] = float(brick->index.y * BRICK_DIAMETER);
      positions[loc * 3 + 2] = float(brick->index.z * BRICK_DIAMETER);

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
    q3Transform tx = this->physicsBody->GetTransform();

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(
      tx.position.x,
      tx.position.y,
      tx.position.z
    ));

//    model = glm::rotate(model, this->rotation.x, glm::vec3(1.0, 0.0, 0.0));
//    model = glm::rotate(model, this->rotation.y, glm::vec3(0.0, 1.0, 0.0));
//    model = glm::rotate(model, this->rotation.z, glm::vec3(0.0, 0.0, 1.0));
//    model = glm::scale(model, scale);

    return model;
  }
};