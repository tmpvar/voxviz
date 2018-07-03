#include "volume.h"
#include "gl-wrap.h"

Volume::Volume(glm::vec3 center, glm::uvec3 dims) {
  this->center = center;
  this->dims = dims;
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
    GL_RGB,
    this->dims.x,
    this->dims.y,
    this->dims.z,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    0
  );

  gl_error();

  cl_int shared_texture_error;

  // Create clgl shared texture
  this->computeBuffer = clCreateFromGLTexture(
    job.context,
    CL_MEM_WRITE_ONLY,
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
  glBindTexture(GL_TEXTURE_3D, this->textureId);
  glActiveTexture(GL_TEXTURE0);

  program->uniform1i("volume", 0)
    ->uniformVec3("center", this->center)
    ->uniformVec3ui("dims", this->dims);

}

void Volume::position(float x, float y, float z) {
  this->center.x = x;
  this->center.y = y;
  this->center.z = z;
}