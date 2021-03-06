#include "fullscreen-surface.h"

FullscreenSurface::FullscreenSurface() {
  this->mesh = new Mesh();
  this->mesh->face(0, 1, 2);

  // a large triangle
  this->mesh
    ->vert(-1, -1, 0)
    ->vert(4, -1, 0)
    ->vert(-1, 4, 0);

  this->mesh->upload();
}

FullscreenSurface::~FullscreenSurface() {
  delete this->mesh;
}

void FullscreenSurface::render(Program *program) {
  this->mesh->render(program, "position");
}
