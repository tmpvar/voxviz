#include "brick.h"
#include "gl-wrap.h"
#include "core.h"
#include "glm/gtc/matrix_transform.hpp"
#include "volume.h"


BrickMemory::BrickMemory() {
  this->ssbo = new SSBO(MAX_BRICKS * sizeof(BrickData));
  gl_error();
}

uint32_t BrickMemory::alloc() {
  return this->alloc_head++;
}

BrickMemory* BrickMemory::_instance = nullptr;
BrickMemory *BrickMemory::instance() {
  if (BrickMemory::_instance == nullptr) {
    BrickMemory::_instance = new BrickMemory();
  }
  return BrickMemory::_instance;
}



Brick::Brick(glm::ivec3 index) {
  this->index = index;
  this->debug = 0.0f;
  this->aabb = aabb_create();
  this->aabb->lower = glm::vec3(index);
  this->aabb->upper = glm::vec3(index) + glm::vec3(1.0);
}

Brick::~Brick() {}

void Brick::createGPUMemory() {
  this->memory_index = BrickMemory::instance()->alloc();
}

void Brick::fill(Program *program) {
  // TODO: ensure the brick has been uploaded prior to filling.
  program
    ->use()
    ->ssbo("brickBuffer", BrickMemory::instance()->ssbo)
    ->uniform1ui("brick_index", memory_index)
    ->compute(glm::uvec3(BRICK_VOXEL_WORDS, 1, 1));
}

void Brick::bind(Program *program) {
}

bool Brick::isect(Brick *other, aabb_t *out) {
  return aabb_isect(this->aabb, other->aabb, out);
}

void Brick::setVoxel(glm::uvec3 pos, float val) {
  if (
    pos.x > BRICK_DIAMETER ||
    pos.y > BRICK_DIAMETER ||
    pos.z > BRICK_DIAMETER
  ) {
    return;
  }

  uint32_t idx = pos.x;
  idx += pos.y * BRICK_DIAMETER;
  idx += pos.z * BRICK_DIAMETER * BRICK_DIAMETER;

  this->data[idx] = val;
  // TODO: maybe only upload the single value that was changed?
  //this->upload();
}

void Brick::fillConst(uint32_t val) {
  // TODO: fix meeeee
  //glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  BrickMemory::instance()->ssbo->bind();

  uint32_t size = sizeof(BrickData);
  uint32_t offset = this->memory_index * size;
  glClearBufferSubData(
    GL_SHADER_STORAGE_BUFFER,
    GL_R32UI,
    offset,
    size,
    GL_RED_INTEGER,
    GL_UNSIGNED_INT,
    &val
  );
  gl_error();
  //this->full = true;
}