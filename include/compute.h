#ifndef __COMPUTE_H__
#define __COMPUTE_H__

  #include "core.h"
  #include "clu.h"
  #include <glm/glm.hpp>

  using namespace cl;

  class Compute {
    protected:
    public:
      // TODO: meh, why does this have to be exposed?
      clu_job_t job;
      cl_mem position_buffer;
    public:
      Compute();
      ~Compute();

      void fill(cl_command_queue queue, cl_mem texture, cl_mem center, int time);
      void lock(cl_command_queue queue, cl_mem texture);
      void unlock(cl_command_queue queue, cl_mem texture);
  };

#endif
