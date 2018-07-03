#include "clu.h"
#include "compute.h"
#include "core.h"
#include "volume.h"

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


void Compute::fill(string kernelName, cl_command_queue queue, Volume* volume, int time) {
  
  const size_t global_threads[3] = { 
    volume->dims.x,
    volume->dims.y,
    volume->dims.z
  };

  cl_kernel kernel = this->job.kernels[kernelName];

  cl_mem texture = volume->computeBuffer;
  cl_mem center = volume->mem_center;

  CL_CHECK_ERROR(clSetKernelArg(kernel, 0, sizeof(texture), &texture));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), &center));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 2, sizeof(int), &time));

  CL_CHECK_ERROR(clEnqueueNDRangeKernel(
    queue,
    kernel,
    3,
    NULL,
    global_threads,
    NULL,
    0,
    NULL, // no waitlist
    NULL  // no callback
  ));
}
