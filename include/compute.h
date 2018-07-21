#ifndef __COMPUTE_H__
#define __COMPUTE_H__

  #include "core.h"
  #include "clu.h"
  #include <glm/glm.hpp>
  #include "volume.h"

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

      void fill(string kernelName, cl_command_queue queue, Volume * volume, int time);
      uint32_t opCut(Volume *target, Volume *cutter);
      void lock(cl_command_queue queue, cl_mem texture);
      void unlock(cl_command_queue queue, cl_mem texture);
  };

#endif
