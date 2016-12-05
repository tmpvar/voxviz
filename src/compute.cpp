#include "clu.h"
#include "compute.h"
#include "core.h"

Compute::Compute() {
  clu_compute_init(&this->job);
}

Compute::~Compute() {
  clu_compute_destroy(&this->job);
}

void Compute::lock(cl_command_queue queue, cl_mem texture) {
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(
    queue,
    1,
    &texture,
    0,
    0,
    NULL
  ));
}

void Compute::unlock(cl_command_queue queue, cl_mem texture) {
  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(
    queue,
    1,
    &texture,
    0,
    0,
    NULL
  ));
}


void Compute::fill(cl_command_queue queue, cl_mem texture, cl_mem center, int time) {
  size_t global_threads[3] = {
    VOLUME_DIMS,
    VOLUME_DIMS,
    VOLUME_DIMS
  };

  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 0, sizeof(texture), &texture));
  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 1, sizeof(cl_mem), &center));
  CL_CHECK_ERROR(clSetKernelArg(this->job.kernel, 2, sizeof(int), &time));

  CL_CHECK_ERROR(clEnqueueNDRangeKernel(
    queue,
    this->job.kernel,
    3,
    NULL,
    global_threads,
    NULL,
    0,
    NULL, // no waitlist
    NULL  // no callback
  ));
}
