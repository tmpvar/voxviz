#ifndef __FULLSCREEN_QUAD_H__
#define __FULLSCREEN_QUAD_H__

#include "gl-wrap.h"

  class FullscreenSurface {
  protected:
    Mesh * mesh;

  public:
    FullscreenSurface();
    ~FullscreenSurface();
    void render(Program *program);
  };
#endif