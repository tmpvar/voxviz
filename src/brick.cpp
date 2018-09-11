#include "brick.h"
#include "gl-wrap.h"
#include "core.h"
#include "glm/gtc/matrix_transform.hpp"

Brick::Brick(glm::ivec3 index) {
  this->index = index;
  this->debug = 0.0f;
  // TODO: we probably only need to malloc this if the data for
  // this brick originates at the HOST
  this->data = (float *)malloc(BRICK_VOXEL_COUNT * sizeof(GLfloat));
}

Brick::~Brick() {
  glMakeBufferNonResidentNV(this->bufferAddress);
  glDeleteBuffers(1, &this->bufferId);
}

void Brick::createGPUMemory() {
  glGenBuffers(1, &bufferId);
  gl_error();

  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();
  // TODO: consider breaking each voxel into 64 bits (4x4x4)
  glBufferData(
    GL_TEXTURE_BUFFER,
    BRICK_VOXEL_COUNT * sizeof(GLfloat),
    this->data,
    GL_STATIC_DRAW
  );
  gl_error();

  glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
  gl_error();

  glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &this->bufferAddress);
  gl_error();
}

void Brick::upload() {
  // TODO: I believe this is causing an exception to be thrown further down the line..
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();
  // TODO: consider breaking each voxel into 64 bits (4x4x4)
  glBufferData(
    GL_TEXTURE_BUFFER,
    BRICK_VOXEL_COUNT * sizeof(GLfloat),
    this->data,
    GL_STATIC_DRAW
  );
}

void Brick::fill(Program *program) {
  // TODO: ensure the brick has been uploaded prior to filling.
  program->use()->bufferAddress("volume", this->bufferAddress);

  glDispatchCompute(
    1,
    BRICK_DIAMETER,
    BRICK_DIAMETER
  );
  gl_error();
}

void Brick::bind(Program *program) {
}

aabb_t Brick::aabb() {
  // TODO: cache aabb and recompute on reposition
  aabb_t ret;
    
  glm::vec3 lower = this->index * BRICK_DIAMETER;

  ret.lower = lower;
  ret.upper = lower + glm::vec3(BRICK_DIAMETER);
  return ret;
}

bool Brick::isect(Brick *other, aabb_t *out) {
  aabb_t a = this->aabb();
  aabb_t b = other->aabb();

  if (
    !(
      (a.lower.x <= b.upper.x && a.upper.x >= b.lower.x) &&
      (a.lower.y <= b.upper.y && a.upper.y >= b.lower.y) &&
      (a.lower.z <= b.upper.z && a.upper.z >= b.lower.z)
     )
  )
  {
    return false;
  }

  out->upper.x = min(a.upper.x, b.upper.x);
  out->upper.y = min(a.upper.y, b.upper.y);
  out->upper.z = min(a.upper.z, b.upper.z);

  out->lower.x = max(a.lower.x, b.lower.x);
  out->lower.y = max(a.lower.y, b.lower.y);
  out->lower.z = max(a.lower.z, b.lower.z);

  return true;
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

void Brick::fillConst(float val) {
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  glClearBufferData(GL_TEXTURE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &val);
}