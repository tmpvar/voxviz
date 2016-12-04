#ifndef __COMPUTE_H__
#define __COMPUTE_H__

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

      void fill(cl_mem texture, glm::vec3, int time);
  };

#endif
