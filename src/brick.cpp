#include "brick.h"
#include "gl-wrap.h"
#include "core.h"
#include "glm/gtc/matrix_transform.hpp"

Brick::Brick(glm::vec3 center) {
  this->center = center;
  this->dims = glm::uvec3(BRICK_DIAMETER);
  this->debug = 0.0f;
}

Brick::~Brick() {
  glMakeBufferNonResidentNV(this->bufferAddress);
  glDeleteBuffers(1, &this->bufferId);
}

void Brick::upload() {
  glGenBuffers(1, &bufferId);
  gl_error();

  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();
  // TODO: consider breaking each voxel into 64 bits (4x4x4)
  glBufferData(
    GL_TEXTURE_BUFFER,
    BRICK_VOXEL_COUNT * sizeof(GLfloat),
    NULL,
    GL_STATIC_DRAW
  );
  gl_error();

  glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
  gl_error();

  glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &this->bufferAddress);
  gl_error();
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

void Brick::position(float x, float y, float z) {
  this->center.x = floorf(x);
  this->center.y = floorf(y);
  this->center.z = floorf(z);
}

void Brick::move(float x, float y, float z) {
  this->center.x += floorf(x);
  this->center.y += floorf(y);
  this->center.z += floorf(z);
}

aabb_t Brick::aabb() {
  // TODO: cache aabb and recompute on reposition
  aabb_t ret;

  glm::vec3 hd = (glm::vec3)this->dims / glm::vec3(2.0, 2.0, 2.0);
  glm::vec3 lower = center - hd;

  ret.lower = lower;
  ret.upper = lower + (glm::vec3)this->dims;
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

void Brick::fillConst(float val) {
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  glClearBufferData(GL_TEXTURE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &val);
}