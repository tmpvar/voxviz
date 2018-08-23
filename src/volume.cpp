#include "volume.h"
#include "gl-wrap.h"

Volume::Volume(glm::vec3 center, glm::uvec3 dims) {
  this->center = center;
  this->dims = dims;
  this->debug = 0.0f;
}

Volume::~Volume() {
  //CL_CHECK_ERROR(clReleaseMemObject(this->mem_center));
  //CL_CHECK_ERROR(clReleaseMemObject(this->computeBuffer));
  glMakeBufferNonResidentNV(this->bufferAddress);
  glDeleteBuffers(1, &this->bufferId);
}

void Volume::upload() {
  glGenBuffers(1, &bufferId);
  gl_error();

  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  gl_error();

  glBufferData(
    GL_TEXTURE_BUFFER,
    this->dims.x * this->dims.y * this->dims.z * sizeof(GLfloat),
    NULL,
    GL_STATIC_DRAW
  );
  gl_error();

  glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
  gl_error();

  glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &this->bufferAddress);
  gl_error();
}

void Volume::bind(Program *program) {
  program
    ->uniformVec3("center", this->center)
    ->uniformVec3ui("dims", this->dims)
    ->uniformFloat("debug", this->debug)
    ->bufferAddress("volume", this->bufferAddress);
  
}

void Volume::bindProxy(Program *program) {
  program
    ->uniformVec3("center", this->center)
    ->uniformVec3ui("dims", this->dims)
    ->uniformFloat("debug", this->debug);
}


void Volume::position(float x, float y, float z) {
  this->center.x = floorf(x);
  this->center.y = floorf(y);
  this->center.z = floorf(z);
}

void Volume::move(float x, float y, float z) {
  this->center.x += floorf(x);
  this->center.y += floorf(y);
  this->center.z += floorf(z);
}

aabb_t Volume::aabb() {
  // TODO: cache aabb and recompute on reposition
  aabb_t ret;

  glm::vec3 hd = (glm::vec3)this->dims / glm::vec3(2.0, 2.0, 2.0);
  glm::vec3 lower = center - hd;


  ret.lower = lower;
  ret.upper = lower + (glm::vec3)this->dims;
  return ret;
}

bool Volume::isect(Volume *other, aabb_t *out) {
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

void Volume::fillConst(float val) {
  glBindBuffer(GL_TEXTURE_BUFFER, bufferId);
  glClearBufferData(GL_TEXTURE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &val);
}