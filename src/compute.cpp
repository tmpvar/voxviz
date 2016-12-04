#include "compute.h"

Compute::Compute() {
  clu_compute_init(&this->job);
}

Compute::~Compute() {
  clu_compute_destroy(&this->job);
}

void Compute::fill(cl_mem texture, glm::vec3 center, int time) {
  size_t global_threads[3] = {
    128,
    128,
    128
  };

  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(
    this->job.command_queue,
    1,
    &texture,
    0,
    0,
    NULL
  ));

  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 0, sizeof(texture), &texture));
  cl_int err;
  cl_mem buffer_b = clCreateBuffer(
    this->job.context,
    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
    sizeof(center),
    &center[0],
    &err
  );

  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 1, sizeof(cl_mem), &buffer_b));

  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 2, sizeof(int), &time));

  CL_CHECK_ERROR(clEnqueueNDRangeKernel(
    this->job.command_queue,
    this->job.kernel,
    3,
    NULL,
    global_threads,
    NULL,
    0,
    NULL, // no waitlist
    NULL  // no callback
  ));

  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(
    this->job.command_queue,
    1,
    &texture,
    0,
    0,
    NULL
  ));

  CL_CHECK_ERROR(clReleaseMemObject(buffer_b));
}