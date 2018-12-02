#include "brick.h"
#include "gl-wrap.h"
#include "core.h"
#include "glm/gtc/matrix_transform.hpp"
#include "volume.h"

Brick::Brick(glm::ivec3 index) {
  this->index = index;
  this->debug = 0.0f;
  this->bufferAddress = 0;
  this->bufferId = 0;
  this->aabb = aabb_create();
  this->aabb->lower = glm::vec3(index);
  this->aabb->upper = glm::vec3(index) + glm::vec3(1.0);
  // TODO: we probably only need to malloc this if the data for
  // this brick originates at the HOST
  //this->data = (float *)malloc(BRICK_VOXEL_COUNT * sizeof(GLfloat));
}

Brick::~Brick() {
  if (this->bufferAddress != 0) {
    glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
    glMakeBufferNonResidentNV(GL_TEXTURE_BUFFER);
    glDeleteBuffers(1, &this->bufferId);
  }
}

void Brick::createGPUMemory() {
  glGenBuffers(1, &bufferId);
  gl_error();

  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();
  // TODO: consider breaking each voxel into 64 bits (4x4x4)
  glBufferData(
    GL_TEXTURE_BUFFER,
    // 4096 * 4 = 16384
    // vs
    // 4096 / 8 = 512
    BRICK_VOXEL_BYTES,//BRICK_VOXEL_COUNT * sizeof(uint32_t),
    NULL,
    GL_STATIC_DRAW
  );
  gl_error();

  glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
  gl_error();

  glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &this->bufferAddress);
  gl_error();

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Brick::upload() {
  // TODO: I believe this is causing an exception to be thrown further down the line..
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();
  // TODO: consider breaking each voxel into 64 bits (4x4x4)
  glBufferData(
    GL_TEXTURE_BUFFER,
    BRICK_VOXEL_BYTES,//BRICK_VOXEL_COUNT * sizeof(uint32_t),
    this->data,
    GL_STATIC_DRAW
  );
}

void Brick::fill(Program *program) {
  // TODO: ensure the brick has been uploaded prior to filling.
  program->use()->bufferAddress("volume", this->bufferAddress);

  glDispatchCompute(
    BRICK_VOXEL_WORDS,
    1,
    1
  );
  gl_error();
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
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  glClearBufferData(GL_TEXTURE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &val);
  this->full = true;
}