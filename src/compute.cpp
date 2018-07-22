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
  cl_event ev;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(
    queue,
    1,
    &texture,
    0,
    nullptr,
    &ev
  ));

  CL_CHECK_ERROR(clWaitForEvents(1, &ev));
  clReleaseEvent(ev);
}

void Compute::unlock(cl_command_queue queue, cl_mem texture) {
  cl_event ev;
  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(
    queue,
    1,
    &texture,
    0,
    nullptr,
    &ev
  ));
  CL_CHECK_ERROR(clWaitForEvents(1, &ev));
  clReleaseEvent(ev);
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

  CL_CHECK_ERROR(clSetKernelArg(kernel, 0, sizeof(cl_mem), &texture));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), &center));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 2, sizeof(int), &time));
  cl_event kernel_completion;

  CL_CHECK_ERROR(clEnqueueNDRangeKernel(
    queue,
    kernel,
    3,
    NULL,
    global_threads,
    NULL,
    0,
    nullptr,
    &kernel_completion  // no callback
  ));
  CL_CHECK_ERROR(clWaitForEvents(1, &kernel_completion));
  clReleaseEvent(kernel_completion);
}

uint32_t Compute::opCut(Volume *target, Volume *cutter) {
  aabb_t overlap;

  if (!cutter->isect(target, &overlap)) {
    target->debug = 0.0;
    return 0;
  }

  //target->debug = 1.0;
  
  glm::vec3 slice = overlap.upper - overlap.lower;

  cl_int3 target_offset;
  aabb_t stock_aabb = target->aabb();
  aabb_t cutter_aabb = cutter->aabb();
  glm::vec3 stock_offset = overlap.lower - stock_aabb.lower;
  target_offset.x = (cl_int)(stock_offset.x);
  target_offset.y = (cl_int)(stock_offset.y);
  target_offset.z = (cl_int)(stock_offset.z);
  target_offset.w = 0;

  glm::vec3 overhang = overlap.lower - cutter_aabb.lower;
  cl_int3 cutter_offset;
  cutter_offset.x = (cl_int)ceilf(overhang.x);
  cutter_offset.y = (cl_int)ceilf(overhang.y);
  cutter_offset.z = (cl_int)ceilf(overhang.z);
  cutter_offset.w = 0;

  cl_kernel kernel = this->job.kernels["opCut"];
  cl_command_queue queue = this->job.command_queues[0];
  cl_mem mem[2] = { target->computeBuffer, cutter->computeBuffer };

  const size_t global_threads[3] = {
    ceilf(slice.x),
    ceilf(slice.y),
    ceilf(slice.z)
  };

  if (global_threads[0] == 0 || global_threads[1] == 0 || global_threads[2] == 0) {
    return 0;
  }

  cl_event opengl_get_completion;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(queue, 2, mem, 0, nullptr, &opengl_get_completion));
  CL_CHECK_ERROR(clWaitForEvents(1, &opengl_get_completion));
  CL_CHECK_ERROR(clReleaseEvent(opengl_get_completion));

  cl_int err;
  cl_mem mem_target_offset = clCreateBuffer(
    this->job.context,
    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
    sizeof(cl_int3),
    &target_offset,
    &err
  );
  CL_CHECK_ERROR(err);

  cl_mem mem_cutter_offset = clCreateBuffer(
    this->job.context,
    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
    sizeof(cl_int3),
    &cutter_offset,
    &err
  );
  CL_CHECK_ERROR(err);


  uint32_t affected = 0;
  cl_mem mem_affected = clCreateBuffer(
    this->job.context,
    CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
    sizeof(cl_uint),
    &affected,
    &err
  );
  CL_CHECK_ERROR(err);


  cl_mem mem_target = target->computeBuffer;
  cl_mem mem_cutter = cutter->computeBuffer;
  CL_CHECK_ERROR(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_target));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_target));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_target_offset));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 3, sizeof(cl_mem), &mem_cutter));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 4, sizeof(cl_mem), &mem_cutter_offset));
  CL_CHECK_ERROR(clSetKernelArg(kernel, 5, sizeof(cl_mem), &mem_affected));

  cl_event kernel_completion;
  err = clEnqueueNDRangeKernel(
    queue,
    kernel,
    3,
    NULL,
    global_threads,
    NULL,
    0,
    nullptr, // no waitlist
    &kernel_completion  // no callback
  );

  if (err != CL_SUCCESS) {
    cout << "err" << endl;
  }

  err = clWaitForEvents(1, &kernel_completion);
  if (err != CL_SUCCESS) {
    cout << "err" << endl;
  }

  CL_CHECK_ERROR(clReleaseEvent(kernel_completion));

  err = clEnqueueReadBuffer(
    queue,
    mem_affected,
    CL_TRUE, // blocking read TODO: does this need to block or can we use a completion event?
    0,
    sizeof(affected),
    &affected,
    0,
    NULL,
    NULL
  );
  CL_CHECK_ERROR(err);


  cl_event opengl_release_completion;
  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(queue, 2, mem, 0, nullptr, &opengl_release_completion));
  CL_CHECK_ERROR(clWaitForEvents(1, &opengl_release_completion));
  CL_CHECK_ERROR(clReleaseEvent(opengl_release_completion));
  
  return affected;
}

