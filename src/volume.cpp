#include "volume.h"
#include "gl-wrap.h"

Volume::Volume(glm::vec3 center, glm::uvec3 dims) {
  this->center = center;
  this->dims = dims;
  this->debug = 0.0f;
}

Volume::~Volume() {
  CL_CHECK_ERROR(clReleaseMemObject(this->mem_center));
  CL_CHECK_ERROR(clReleaseMemObject(this->computeBuffer));
  glDeleteTextures(1, &this->textureId);
}

void Volume::upload(clu_job_t job) {
  glGenTextures(1, &this->textureId);
  gl_error();
  glBindTexture(GL_TEXTURE_3D, this->textureId);
  gl_error();

  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexImage3D(
    GL_TEXTURE_3D,
    0,
    GL_RED,
    this->dims.x,
    this->dims.y,
    this->dims.z,
    0,
    GL_RED,
    GL_UNSIGNED_BYTE,
    0
  );

  gl_error();

  cl_int shared_texture_error;

  // Create clgl shared texture
  this->computeBuffer = clCreateFromGLTexture(
    job.context,
    CL_MEM_READ_WRITE,
    GL_TEXTURE_3D,
    0,
    this->textureId,
    &shared_texture_error
  );

  clu_error(shared_texture_error);

  cl_int err;
  this->mem_center = clCreateBuffer(
    job.context,
    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
    sizeof(this->center),
    &this->center[0],
    &err
  );

  CL_CHECK_ERROR(err);

  this->mem_dims = clCreateBuffer(
    job.context,
    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
    sizeof(this->dims),
    &this->dims[0],
    &err
  );
  CL_CHECK_ERROR(err);
}

void Volume::bind(Program *program) {
  program->texture3d("volume", this->textureId)
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