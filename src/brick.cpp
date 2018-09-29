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
  this->data = (float *)malloc(BRICK_VOXEL_COUNT * sizeof(GLfloat));
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
    BRICK_VOXEL_COUNT * sizeof(GLfloat),
    this->data,
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

void Brick::fillConst(float val) {
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  glClearBufferData(GL_TEXTURE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &val);
}


void Brick::cut(Brick *cutter, Program *program) {
  // compute overlapping aabb
  // kick off a compute job
  glm::vec3 volumeOffset;
  glm::vec3 cutterOffset;
  glm::ivec3 sliceDims;

  this->volume->computeBrickOffset(
    this,
    cutter,
    &volumeOffset,
    &cutterOffset,
    &sliceDims
  );

  if (program != nullptr) {
    program->use()
      ->bufferAddress("volume", this->bufferAddress)
      ->bufferAddress("cutter", cutter->bufferAddress)
      ->uniformVec3ui("volumeOffset", glm::uvec3(volumeOffset))
      ->uniformVec3ui("cutterOffset", glm::uvec3(cutterOffset));

    /*
    // this is currently triggering a gl error which causes the process to die
    sliceDims = glm::clamp(sliceDims, glm::uvec3(1), glm::uvec3(BRICK_DIAMETER - 1));
    glDispatchComputeGroupSizeARB(
      1,
      1,
      1,
      sliceDims.x,
      sliceDims.y,
      sliceDims.z
    );*/

    glDispatchCompute(
      1,
      BRICK_DIAMETER,
      BRICK_DIAMETER
    );
 
    gl_error();
  }
}