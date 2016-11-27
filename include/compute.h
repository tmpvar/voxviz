#ifndef __COMPUTE_H__
#define __COMPUTE_H__

  #include "cl/clu.h"

  using namespace cl;

  class Compute {
    protected:
    public:
      // TODO: meh, why does this have to be exposed?
      clu_job_t job;
    public:
      Compute();
      ~Compute();

      void fill(cl_mem texture) {
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

        clSetKernelArg(this->job.kernel, 0, sizeof(texture), &texture);
        static int time = 0;
        time ++;
        clSetKernelArg(this->job.kernel, 1, sizeof(int), &time);

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
      }
  };

#endif
